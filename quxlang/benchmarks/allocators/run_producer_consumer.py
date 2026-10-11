#!/usr/bin/env python3
"""Compare remote deallocation using producer-consumer pairs and bounded queues."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import ROOT, REPOSITORY, digest


def reference(batches, pairs):
    """Compute expected pair checksums independently with explicit U64 reduction."""
    mask = (1 << 64) - 1
    results = []
    for identity in range(pairs):
        state = 20261011 + identity
        first, second = 14695981039346656037, 1099511628211
        for index in range(batches * 128):
            words = []
            for field in range(2):
                state = (state + 11400714819323198485) & mask
                value = state
                value = ((value ^ (value >> 30)) * 13787848793156543929) & mask
                value = ((value ^ (value >> 27)) * 10723151780598845931) & mask
                words.append(value ^ (value >> 31))
            multiplier = words[1] | 1
            first = ((first ^ words[0]) * multiplier) & mask
            second = ((second ^ words[0]) * multiplier) & mask
        results.append([identity, first, second])
    return results


def main():
    """Build Release binaries, verify every checksum, and measure interleaved repetitions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'allocator-comparison-out/producer-consumer')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--batches', type=int, default=8192, help='128-object batches produced by each pair.')
    parser.add_argument('--pairs', type=int, nargs='+', default=[1, 8, 16])
    parser.add_argument('--repetitions', type=int, default=3)
    args = parser.parse_args()
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        parser.error('this runner currently measures native macOS ARM64')
    if args.batches < 1 or args.repetitions < 1 or any(count < 1 for count in args.pairs):
        parser.error('batches, repetitions and pair counts must be positive')
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('std', 'runtime', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/main', bundle / 'modules/main', dirs_exist_ok=True)
    config = (ROOT / 'qxcbuild.yml').read_text().replace('benchmark: "::allocations"', 'benchmark: "::producer_consumer"')
    (bundle / 'qxcbuild.yml').write_text(config)
    qxc = REPOSITORY / 'misc/build/buildspaces/system-clang/quxlang/Release/qxc'
    mimalloc = args.mimalloc_source.resolve()
    commands = []

    def execute(command, **kwargs):
        """Record and execute a build command."""
        commands.append([str(value) for value in command])
        subprocess.run(command, check=True, **kwargs)

    with (work / 'qxc-build.log').open('w') as log:
        execute([qxc, bundle, work / 'quxlang', 'native'], stdout=log, stderr=subprocess.STDOUT)
    execute(['clang', '-std=c17', '-O3', '-DNDEBUG', '-DMI_STATIC_LIB', '-I' + str(mimalloc / 'include'),
             '-c', mimalloc / 'src/static.c', '-o', work / 'mimalloc.o'])
    execute(['clang++', '-std=c++17', '-O3', '-DNDEBUG', '-DMI_STATIC_LIB', '-pthread',
             '-fno-builtin-malloc', '-fno-builtin-free', '-I' + str(mimalloc / 'include'),
             ROOT / 'producer_consumer.cpp', ROOT / 'barriers.S', work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    executables = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = subprocess.check_output(['xcrun', '--show-sdk-version'], text=True).strip()
    for executable in executables.values():
        execute(['xcrun', 'vtool', '-set-build-version', 'macos', '11.0', sdk, '-replace', '-output', executable, executable])
        execute(['codesign', '--force', '--sign', '-', executable])
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'commands': commands, 'batches_per_pair': args.batches,
              'objects_per_batch': 128, 'object_bytes': 16, 'queue_capacity_batches': 64,
              'generator': 'SplitMix64', 'pair_seed': '20261011 + pair_id',
              'checksum': 'state = (state XOR first) * (second OR 1), modulo 2^64; initial states 14695981039346656037 and 1099511628211',
              'timing': 'Thread creation through all joins, including consumer checksum output and TLS teardown; queue construction and destruction excluded.',
              'counter': 'ISB; MRS X0, CNTVCT_EL0; ISB; RET; frequency read from CNTFRQ_EL0 independently in each binary',
              'repetitions': args.repetitions, 'discarded_warmup_runs': 1,
              'qxc_sha256': digest(qxc), 'build_config_sha256': digest(bundle / 'qxcbuild.yml'),
              'binary_sha256': {name: digest(path) for name, path in executables.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': digest(ROOT / 'producer_consumer.cpp'), 'barriers_sha256': digest(ROOT / 'barriers.S'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'results': []}
    for pairs in args.pairs:
        expected = reference(args.batches, pairs)
        samples = {name: [] for name in executables}
        raw = []
        total_objects = pairs * args.batches * 128
        for repeat in range(args.repetitions + 1):
            order = list(executables)
            if repeat % 2:
                order.reverse()
            for name in order:
                options = [f'--batches={args.batches}', f'--pairs={pairs}'] if name == 'quxlang_alloc' else [str(args.batches), str(pairs)]
                output = subprocess.check_output([executables[name], *options], env=environment, timeout=1200, text=True)
                lines = [list(map(int, line.split())) for line in output.splitlines() if line.strip()]
                if len(lines) != pairs + 1 or sorted(lines[:-1]) != expected or len(lines[-1]) != 2:
                    raise RuntimeError(f'{name}: invalid checksums or output: {output}')
                ticks, frequency = lines[-1]
                if ticks <= 0 or frequency <= 0:
                    raise RuntimeError(f'{name}: invalid timer: {lines[-1]}')
                ns = ticks * 1e9 / frequency / total_objects
                raw.append({'allocator': name, 'run': repeat, 'ticks': ticks, 'frequency_hz': frequency, 'checksums': lines[:-1]})
                if repeat:
                    samples[name].append(ns)
                print(f'pairs={pairs} run={repeat} allocator={name} ns/object={ns:.3f}', flush=True)
        medians = {name: statistics.median(values) for name, values in samples.items()}
        row = {'pairs': pairs, 'threads': pairs * 2, 'total_objects': total_objects,
               'samples_ns_per_object': samples, 'median_ns_per_object': medians,
               'quxlang_percent_of_mimalloc': medians['quxlang_alloc'] * 100 / medians['cpp_mimalloc'],
               'independent_reference': expected, 'runs': raw}
        report['results'].append(row)
        (work / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({key: value for key, value in row.items() if key not in ('runs', 'independent_reference')}), flush=True)


if __name__ == '__main__':
    main()
