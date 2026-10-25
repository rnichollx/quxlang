#!/usr/bin/env python3
"""The runner compares small-allocation lifetimes with matched C++ workloads and verifies payload checksums."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import add_cpp_build_arguments, cpp_build_flags, ROOT, REPOSITORY, counter_description, digest, host_configuration, measure_case, minimum_seconds, prepare_executables


PATTERNS = {'fifo': 0, 'lifo': 1, 'rolling': 2}
SIZES = (16, 64, 256, 1024, 4096, 8192)


def main():
    """The runner builds both implementations and records interleaved samples of identical work."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'tmp/allocator-comparison-out/lifetimes')
    parser.add_argument('--qxc', type=Path, default=REPOSITORY / 'misc/build/buildspaces/system-clang-libcxx/quxlang/Release/qxc')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--sizes', type=int, nargs='+', choices=SIZES, default=list(SIZES))
    parser.add_argument('--windows', type=int, nargs='+', default=[64, 4096])
    parser.add_argument('--threads', type=int, nargs='+', default=[1, 16, 128])
    parser.add_argument('--patterns', nargs='+', choices=PATTERNS, default=list(PATTERNS))
    parser.add_argument('--operations', type=int, default=1000000)
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--min-seconds', type=minimum_seconds, default=3.0)
    parser.add_argument('--build-only', action='store_true')
    add_cpp_build_arguments(parser)
    args = parser.parse_args()
    if args.operations < 1 or args.repetitions < 1 or any(t < 1 or t > 128 for t in args.threads) or any(w < 1 or w > 4096 for w in args.windows):
        parser.error('operations and repetitions must be positive; threads must be 1..128 and windows 1..4096')
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('std', 'runtime', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/main', bundle / 'modules/main', dirs_exist_ok=True)
    config = (ROOT / 'qxcbuild.yml').read_text().replace('benchmark: "::allocations"', 'benchmark: "::lifetimes"')
    (bundle / 'qxcbuild.yml').write_text(host_configuration(config))
    commands = []

    def execute(command, **kwargs):
        commands.append([str(value) for value in command])
        subprocess.run(command, check=True, **kwargs)

    qxc = args.qxc.resolve()
    mimalloc = args.mimalloc_source.resolve()
    cpp_flags, cpp_linker_flags = cpp_build_flags(args)
    with (work / 'qxc-build.log').open('w') as log:
        execute([qxc, bundle, work / 'quxlang', 'native'], stdout=log, stderr=subprocess.STDOUT)
    execute(['clang', '-std=c17', '-O3', *cpp_flags, '-DNDEBUG', '-DMI_STATIC_LIB', '-I' + str(mimalloc / 'include'),
             '-c', mimalloc / 'src/static.c', '-o', work / 'mimalloc.o'])
    execute(['clang++', '-std=c++17', '-O3', *cpp_flags, *cpp_linker_flags, '-fsized-deallocation', '-DNDEBUG', '-DUSE_MIMALLOC', '-DMI_STATIC_LIB', '-pthread',
             '-fno-builtin-malloc', '-fno-builtin-free', '-I' + str(mimalloc / 'include'),
             ROOT / 'lifetimes.cpp', ROOT / 'barriers.S', work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    binaries = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = prepare_executables(binaries, execute)
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'logical_cpus': os.cpu_count(), 'commands': commands,
              'cpp_target': args.cpp_target, 'cpp_lto': args.cpp_lto,
              'clang': subprocess.check_output(['clang', '--version'], text=True),
              'workload': 'FIFO and LIFO allocate complete windows before freeing; rolling replaces one oldest allocation at a time while retaining the rest. Every allocation receives a distinct sequence number through one opaque word write and contributes its payload word to the free-time checksum.',
              'timing': 'Pointer-window storage is created outside timing. Parallel workers warm up before release; measured work ends at the last worker completion and excludes teardown. Single-thread runs measure the allocation loop directly.',
              'counter': counter_description(), 'sdk_version': sdk, 'minimum_sample_seconds': args.min_seconds,
              'repetitions': args.repetitions, 'qxc_sha256': digest(qxc), 'measurement_sha256': digest(ROOT.parent / 'measurement.py'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'binary_sha256': {name: digest(path) for name, path in binaries.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': digest(ROOT / 'lifetimes.cpp'), 'results': []}
    destination = work / 'results.json'
    destination.write_text(json.dumps(report, indent=2) + '\n')
    if args.build_only:
        return
    for threads in args.threads:
        for size in args.sizes:
            for window in args.windows:
                for pattern in args.patterns:
                    def sample(name, count):
                        options = [f'--n={count}', f'--size={size}', f'--batch={window}', f'--threads={threads}', f'--pattern={PATTERNS[pattern]}'] if name == 'quxlang_alloc' else [str(count), str(size), str(window), str(threads), str(PATTERNS[pattern])]
                        output = subprocess.check_output([binaries[name], *options], text=True, env=environment, timeout=1200)
                        ticks, frequency, checksum = map(int, output.split())
                        if checksum != (count * (count + 1) // 2 * threads) % (1 << 64) or ticks <= 0 or frequency <= 0:
                            raise RuntimeError(f'{name}: invalid checksum or timer: {output!r}')
                        seconds = ticks / frequency
                        print(f'{pattern} T={threads} bytes={size} window={window} {name} n={count} seconds={seconds:.6f}', flush=True)
                        return {'seconds': seconds, 'frequency_hz': frequency, 'checksum': checksum}

                    result = measure_case(binaries, sample, args.operations, args.repetitions, args.min_seconds)
                    samples = {name: [row['seconds'] for row in rows] for name, rows in result['samples'].items()}
                    report['results'].append({'threads': threads, 'bytes': size, 'window': window, 'pattern': pattern,
                                              'allocations_per_thread': result['work'], 'sample_seconds': samples,
                                              'median_seconds': {name: statistics.median(rows) for name, rows in samples.items()},
                                              'calibration': result['calibration'], 'run_order': result['run_order'],
                                              'expected_checksum': (result['work'] * (result['work'] + 1) // 2 * threads) % (1 << 64)})
                    destination.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
