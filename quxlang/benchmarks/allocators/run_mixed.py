#!/usr/bin/env python3
"""Measure shared MT19937 allocation toggles in Quxlang and C++ Mimalloc."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import ROOT, REPOSITORY, digest


def main():
    """Build Release binaries and measure repeated runs after one discarded warmup."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'allocator-comparison-out/mixed')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--operations', type=int, default=8000000, help='Operations executed by each worker.')
    parser.add_argument('--slots', type=int, nargs='+', default=[4096, 65536])
    parser.add_argument('--threads', type=int, nargs='+', default=[1])
    parser.add_argument('--repetitions', type=int, default=3)
    args = parser.parse_args()
    if platform.system() != 'Darwin' or platform.machine() != 'arm64':
        parser.error('this runner currently measures native macOS ARM64')
    if args.operations < 1:
        parser.error('--operations must be positive')
    if args.repetitions < 1:
        parser.error('--repetitions must be positive')
    if any(slots < 1 or slots > 2**29 for slots in args.slots):
        parser.error('--slots must be between 1 and 536870912')
    if any(count < 1 or count > min(args.slots) for count in args.threads):
        parser.error('--threads must be positive and no greater than slots')
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('std', 'runtime', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/main', bundle / 'modules/main', dirs_exist_ok=True)
    config = (ROOT / 'qxcbuild.yml').read_text().replace('benchmark: "::allocations"', 'benchmark: "::mixed_allocations"')
    (bundle / 'qxcbuild.yml').write_text(config)
    qxc = REPOSITORY / 'misc/build/buildspaces/system-clang/quxlang/Release/qxc'
    commands = []

    def execute(command, **kwargs):
        """Record and execute a build or input-generation command."""
        commands.append([str(value) for value in command])
        subprocess.run(command, check=True, **kwargs)

    with (work / 'qxc-build.log').open('w') as log:
        execute([qxc, bundle, work / 'quxlang', 'native'], stdout=log, stderr=subprocess.STDOUT)
    mimalloc = args.mimalloc_source.resolve()
    execute(['clang', '-std=c17', '-O3', '-DNDEBUG', '-DMI_STATIC_LIB', '-I' + str(mimalloc / 'include'),
             '-c', mimalloc / 'src/static.c', '-o', work / 'mimalloc.o'])
    execute(['clang++', '-std=c++17', '-O3', '-DNDEBUG', '-DMI_STATIC_LIB', '-pthread',
             '-fno-builtin-malloc', '-fno-builtin-free', '-I' + str(mimalloc / 'include'),
             ROOT / 'mixed_allocations.cpp', ROOT / 'barriers.S', work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    executables = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = subprocess.check_output(['xcrun', '--show-sdk-version'], text=True).strip()
    for executable in executables.values():
        execute(['xcrun', 'vtool', '-set-build-version', 'macos', '11.0', sdk, '-replace', '-output', executable, executable])
        execute(['codesign', '--force', '--sign', '-', executable])
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'commands': commands, 'operations_per_thread': args.operations,
              'sizes': [8, 16, 24, 32, 48, 64, 128, 256], 'seed': 20261009, 'generator': 'std::mt19937',
              'sequences': {}, 'selection': 'size = word % 8; slot = (word / 8) % worker_slots',
              'partition': 'Only slots are divided: floor(K / T), with one extra for the first K % T workers; every worker executes N operations',
              'worker_seed': '20261009 + worker_index', 'sequence_checksum_algorithm': 'XOR of all U32 words',
              'unit': 'ns per allocation or deallocation operation', 'repetitions': args.repetitions, 'discarded_warmup_runs': 1,
              'payload_write': 'Store byte value 1 at offset zero immediately after every allocation using matching opaque assembly calls.',
              'timing': 'Thread creation through all joins, including worker TLS teardown; input loading, null initialization and final cleanup excluded.',
              'counter': 'ISB; MRS X0, CNTVCT_EL0; ISB; RET', 'sdk_version': sdk,
              'qxc_sha256': digest(qxc), 'build_config_sha256': digest(bundle / 'qxcbuild.yml'),
              'binary_sha256': {name: digest(path) for name, path in executables.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': digest(ROOT / 'mixed_allocations.cpp'), 'barriers_sha256': digest(ROOT / 'barriers.S'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'results': []}
    for threads in args.threads:
        sequence = work / f'mt19937-t{threads}.bin'
        with sequence.open('wb') as output:
            execute([executables['cpp_mimalloc'], str(args.operations), '0', str(threads)], stdout=output)
        total_operations = args.operations * threads
        assert sequence.stat().st_size == total_operations * 4
        sequence_hash = hashlib.sha256()
        with sequence.open('rb') as source:
            for chunk in iter(lambda: source.read(8 * 1024 * 1024), b''):
                sequence_hash.update(chunk)
        report['sequences'][str(threads)] = {'path': str(sequence), 'sha256': sequence_hash.hexdigest()}
        for slots in args.slots:
            samples = {name: [] for name in executables}
            validation = None
            for repeat in range(args.repetitions + 1):
                order = list(executables)
                if repeat % 2:
                    order.reverse()
                for name in order:
                    options = [f'--n={args.operations}', f'--slots={slots}', f'--threads={threads}'] if name == 'quxlang_alloc' else [str(args.operations), str(slots), str(threads)]
                    with sequence.open('rb') as source:
                        output = subprocess.check_output([executables[name], *options], stdin=source, env=environment, timeout=1200)
                    ticks, frequency, checksum, live, live_bytes = map(int, output.split())
                    assert ticks > 0 and frequency == 1000000000
                    if validation is None:
                        validation = (checksum, live, live_bytes)
                    assert validation == (checksum, live, live_bytes), (name, validation, checksum, live)
                    print(f"T={threads} K={slots} run={repeat} allocator={name} ns/op={ticks * 1e9 / frequency / total_operations:.3f} live_GiB={live_bytes / 2**30:.6f}", flush=True)
                    if repeat:
                        samples[name].append(ticks * 1e9 / frequency / total_operations)
            medians = {name: statistics.median(values) for name, values in samples.items()}
            elapsed_seconds = {name: value * total_operations / 1e9 for name, value in medians.items()}
            row = {'threads': threads, 'total_operations': total_operations, 'slots_per_size': slots, 'samples_ns_per_operation': samples, 'median_ns_per_operation': medians,
                   'median_elapsed_seconds': elapsed_seconds, 'counter_frequency_hz': frequency, 'sequence_checksum': validation[0], 'remaining_live_allocations': validation[1],
                   'remaining_live_bytes': validation[2],
                   'percentage': 100 * medians['quxlang_alloc'] / medians['cpp_mimalloc']}
            report['results'].append(row)
            (work / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
            print(json.dumps(row), flush=True)

if __name__ == '__main__':
    main()
