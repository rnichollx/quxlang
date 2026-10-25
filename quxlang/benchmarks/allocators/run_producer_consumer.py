#!/usr/bin/env python3
"""The runner compares remote deallocation using repeated producer-consumer work passes."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import add_cpp_build_arguments, cpp_build_flags, ROOT, REPOSITORY, digest, host_configuration, prepare_executables, counter_description, measure_case, minimum_seconds


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
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'tmp/allocator-comparison-out/producer-consumer')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--batches', type=int, default=8192, help='128-object batches per pair and pass; calibration increases the pass count.')
    parser.add_argument('--pairs', type=int, nargs='+', default=[1, 8, 16])
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--qxc', type=Path, default=REPOSITORY / 'misc/build/buildspaces/system-clang/quxlang/Release/qxc')
    parser.add_argument("--min-seconds", type=minimum_seconds, default=3.0, help="Minimum seconds per recorded sample (at least 3); calibration trials may be shorter.")
    add_cpp_build_arguments(parser)
    args = parser.parse_args()
    cpp_flags, cpp_linker_flags = cpp_build_flags(args)
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
    (bundle / 'qxcbuild.yml').write_text(host_configuration(config))
    qxc = args.qxc.resolve()
    mimalloc = args.mimalloc_source.resolve()
    commands = []

    def execute(command, **kwargs):
        """Record and execute a build command."""
        commands.append([str(value) for value in command])
        subprocess.run(command, check=True, **kwargs)

    with (work / 'qxc-build.log').open('w') as log:
        execute([qxc, bundle, work / 'quxlang', 'native'], stdout=log, stderr=subprocess.STDOUT)
    execute(['clang', '-std=c17', '-O3', *cpp_flags, '-DNDEBUG', '-DMI_STATIC_LIB', '-I' + str(mimalloc / 'include'),
             '-c', mimalloc / 'src/static.c', '-o', work / 'mimalloc.o'])
    execute(['clang++', '-std=c++17', '-O3', *cpp_flags, *cpp_linker_flags, '-fsized-deallocation', '-DNDEBUG', '-DMI_STATIC_LIB', '-pthread',
             '-fno-builtin-malloc', '-fno-builtin-free', '-I' + str(mimalloc / 'include'),
             ROOT / 'producer_consumer.cpp', ROOT / 'barriers.S', work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    executables = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = prepare_executables(executables, execute)
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'cpp_target': args.cpp_target, 'cpp_lto': args.cpp_lto, 'commands': commands, 'batches_per_pass_per_pair': args.batches, 'minimum_sample_seconds': args.min_seconds,
              'measurement_sha256': digest(ROOT.parent / 'measurement.py'),
              'pass_protocol': 'Each pass restarts the deterministic input and contributes its checksums through XOR. Threads and queues persist between passes; odd pass counts preserve the independent reference.',
              'objects_per_batch': 128, 'object_bytes': 16, 'queue_capacity_batches': 64,
              'generator': 'SplitMix64', 'pair_seed': '20261011 + pair_id',
              'checksum': 'state = (state XOR first) * (second OR 1), modulo 2^64; initial states 14695981039346656037 and 1099511628211',
              'timing': 'Thread creation through all joins, including consumer checksum output and TLS teardown; queue construction and destruction excluded.',
              'counter': counter_description(), 'sdk_version': sdk,
              'repetitions': args.repetitions, 'discarded_warmup_runs': 1,
              'qxc_sha256': digest(qxc), 'build_config_sha256': digest(bundle / 'qxcbuild.yml'),
              'binary_sha256': {name: digest(path) for name, path in executables.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': digest(ROOT / 'producer_consumer.cpp'), 'barriers_sha256': digest(ROOT / 'barriers.S'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'results': []}
    for pairs in args.pairs:
        expected = reference(args.batches, pairs)
        frequencies = set()

        def execute_sample(name, passes):
            options = [f'--batches={args.batches}', f'--pairs={pairs}', f'--passes={passes}'] if name == 'quxlang_alloc' else [str(args.batches), str(pairs), str(passes)]
            output = subprocess.check_output([executables[name], *options], env=environment, timeout=1200, text=True)
            lines = [list(map(int, line.split())) for line in output.splitlines() if line.strip()]
            if len(lines) != pairs + 1 or sorted(lines[:-1]) != expected or len(lines[-1]) != 2:
                raise RuntimeError(f'{name}: invalid checksums or output: {output}')
            ticks, frequency = lines[-1]
            if ticks <= 0 or frequency <= 0:
                raise RuntimeError(f'{name}: invalid timer: {lines[-1]}')
            frequencies.add(frequency)
            if len(frequencies) != 1:
                raise RuntimeError('benchmark timer frequencies differ or changed')
            duration = ticks / frequency
            print(f'pairs={pairs} allocator={name} passes={passes} seconds={duration:.6f}', flush=True)
            return {'seconds': duration, 'ticks': ticks, 'frequency_hz': frequency, 'checksums': lines[:-1]}

        measurement = measure_case(executables, execute_sample, 1, args.repetitions, args.min_seconds, odd_work=True)
        passes = measurement['work']
        total_objects = pairs * args.batches * 128 * passes
        elapsed_samples = {name: [sample['seconds'] for sample in values] for name, values in measurement['samples'].items()}
        samples = {name: [duration * 1e9 / total_objects for duration in values] for name, values in elapsed_samples.items()}
        raw = [{'allocator': name, 'run': index + 1, **sample} for name, values in measurement['samples'].items() for index, sample in enumerate(values)]
        medians = {name: statistics.median(values) for name, values in samples.items()}
        row = {'pairs': pairs, 'threads': pairs * 2, 'total_objects': total_objects,
               'passes': passes, 'batches_per_pair': args.batches * passes,
               'sample_seconds': elapsed_samples, 'calibration': measurement['calibration'],
               'samples_ns_per_object': samples, 'median_ns_per_object': medians,
               'quxlang_percent_of_mimalloc': medians['quxlang_alloc'] * 100 / medians['cpp_mimalloc'],
               'independent_reference': expected, 'runs': raw}
        report['results'].append(row)
        (work / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({key: value for key, value in row.items() if key not in ('runs', 'independent_reference')}), flush=True)


if __name__ == '__main__':
    main()
