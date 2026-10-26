#!/usr/bin/env python3
"""The runner compares complete tree allocation and recursive deallocation at a fixed memory budget."""

import argparse
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import add_cpp_build_arguments, cpp_build_flags, ROOT, REPOSITORY, counter_description, digest, host_configuration, measure_case, minimum_seconds, prepare_executables


SIZES = (32, 64, 256, 1024, 4096, 8192)
U64_MODULUS = 1 << 64


def read_sample(output, size, memory, cycles):
    """The decoder verifies phase timers, the exact release count, and an independent payload checksum."""
    allocated, deallocated, frequency, checksum, freed = map(int, output.split())
    nodes = memory // size
    payload_words = size // 8 - 2
    observed_words = (payload_words + 511) // 512 + 1
    expected = (nodes * (nodes + 1) // 2 * observed_words * cycles) % U64_MODULUS
    if allocated <= 0 or deallocated <= 0 or frequency <= 0 or checksum != expected or freed != nodes * cycles:
        raise RuntimeError(f'invalid bulk lifetime result: {output!r}; expected checksum={expected}, freed={nodes * cycles}')
    allocation_seconds = allocated / frequency
    deallocation_seconds = deallocated / frequency
    return {'seconds': min(allocation_seconds, deallocation_seconds),
            'allocation_seconds': allocation_seconds, 'deallocation_seconds': deallocation_seconds,
            'total_seconds': allocation_seconds + deallocation_seconds,
            'frequency_hz': frequency, 'checksum': checksum, 'freed_nodes': freed}


def main():
    """The runner calibrates cycle counts while preserving the requested peak live-node budget."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'tmp/allocator-comparison-out/bulk-lifetimes')
    parser.add_argument('--qxc', type=Path, default=REPOSITORY / 'misc/build/buildspaces/system-clang-libcxx/quxlang/Release/qxc')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--sizes', type=int, nargs='+', choices=SIZES, default=[64, 1024, 8192])
    parser.add_argument('--memory-mib', type=int, default=16384,
                        help='Peak live node object bytes in MiB, including child pointers; allocator overhead is additional. Default: 16 GiB.')
    parser.add_argument('--cycles', type=int, default=1, help='Initial complete build/destroy cycles; calibration increases cycles, not memory.')
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--min-seconds', type=minimum_seconds, default=3.0,
                        help='Minimum accumulated time in each phase of every recorded sample (at least three seconds).')
    parser.add_argument('--timeout', type=int, default=3600, help='Maximum seconds for each executable invocation.')
    parser.add_argument('--build-only', action='store_true')
    add_cpp_build_arguments(parser)
    args = parser.parse_args()
    memory = args.memory_mib * 1024 * 1024
    if args.memory_mib < 1 or memory > (1 << 63) - 1 or args.cycles < 1 or args.repetitions < 1 or args.timeout < 1:
        parser.error('memory must be a positive MiB count below 2^63 bytes; cycles, repetitions, and timeout must be positive')
    if platform.architecture()[0] != '64bit':
        parser.error('bulk lifetime benchmarks require a 64-bit host')
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('std', 'runtime', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/main', bundle / 'modules/main', dirs_exist_ok=True)
    config = (ROOT / 'qxcbuild.yml').read_text().replace('benchmark: "::allocations"', 'benchmark: "::bulk_lifetimes"')
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
    execute(['clang++', '-std=c++17', '-O3', *cpp_flags, *cpp_linker_flags, '-fsized-deallocation', '-DNDEBUG',
             '-DUSE_MIMALLOC', '-DMI_STATIC_LIB', '-fno-builtin-malloc', '-fno-builtin-free', '-pthread',
             '-I' + str(mimalloc / 'include'), ROOT / 'bulk_lifetimes.cpp', ROOT / 'barriers.S',
             work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    binaries = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = prepare_executables(binaries, execute)
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'logical_cpus': os.cpu_count(), 'threads': 1, 'commands': commands,
              'cpp_target': args.cpp_target, 'cpp_lto': args.cpp_lto, 'sdk_version': sdk,
              'clang': subprocess.check_output(['clang', '--version'], text=True), 'counter': counter_description(),
              'workload': 'Each cycle builds an exact-count balanced binary tree in preorder without freeing any nodes, then recursively deletes all nodes in postorder. Both drivers zero-initialize nodes. Consecutive payload identities are written through opaque barriers at 4096-byte intervals and at the final payload word, and checked during deletion.',
              'memory_definition': 'The budget covers simultaneously live node object bytes, including two child pointers. Allocator metadata, retained free memory, and process overhead are additional. Calibration repeats complete cycles without increasing the node count.',
              'timing': 'Allocation includes construction, linking, and payload writes. Deallocation includes recursive traversal, payload checksums, and delete. Both phases are timed separately on the main thread, including first-use allocation costs. Per-cycle timers exclude process startup, reporting, and thread-local teardown.',
              'calibration_duration_definition': 'The seconds field is the shorter accumulated phase duration; every recorded sample therefore has at least min-seconds in each phase.',
              'memory_budget_bytes': memory, 'minimum_phase_seconds': args.min_seconds, 'repetitions': args.repetitions,
              'qxc_sha256': digest(qxc), 'measurement_sha256': digest(ROOT.parent / 'measurement.py'),
              'runner_sha256': digest(Path(__file__)), 'mimalloc_object_sha256': digest(work / 'mimalloc.o'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'binary_sha256': {name: digest(path) for name, path in binaries.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': {name: digest(ROOT / name) for name in ('bulk_lifetimes.cpp', 'barriers.S', 'barriers.h')},
              'verification': [], 'results': []}
    destination = work / 'results.json'
    destination.write_text(json.dumps(report, indent=2) + '\n')
    if args.build_only:
        return

    frequencies = {}

    def sample(name, cycles, size, budget):
        options = [f'--n={cycles}', f'--size={size}', f'--memory={budget}'] if name == 'quxlang_alloc' else [str(cycles), str(size), str(budget)]
        output = subprocess.check_output([binaries[name], *options], text=True, env=environment, timeout=args.timeout)
        result = read_sample(output, size, budget, cycles)
        previous = frequencies.setdefault(name, result['frequency_hz'])
        if previous != result['frequency_hz'] or len(set(frequencies.values())) != 1:
            raise RuntimeError('benchmark timer frequencies differ or changed')
        return result

    for size in args.sizes:
        for nodes in (1, 2, 3, 31, 32, 33):
            budget = nodes * size + size - 1
            for name in binaries:
                result = sample(name, 2, size, budget)
                report['verification'].append({'implementation': name, 'node_bytes': size, 'nodes': nodes,
                                               'cycles': 2, 'checksum': result['checksum'], 'freed_nodes': result['freed_nodes']})
        destination.write_text(json.dumps(report, indent=2) + '\n')

        def measured_sample(name, cycles):
            result = sample(name, cycles, size, memory)
            print(f'bulk bytes={size} live_bytes={memory // size * size} {name} cycles={cycles} '
                  f'allocate={result["allocation_seconds"]:.6f}s deallocate={result["deallocation_seconds"]:.6f}s', flush=True)
            return result

        result = measure_case(binaries, measured_sample, args.cycles, args.repetitions, args.min_seconds)
        nodes = memory // size
        medians = {phase: {name: statistics.median(row[phase] / result['work'] for row in rows)
                           for name, rows in result['samples'].items()}
                   for phase in ('allocation_seconds', 'deallocation_seconds', 'total_seconds')}
        report['results'].append({'node_bytes': size, 'nodes_per_cycle': nodes, 'live_node_bytes': nodes * size,
                                  'cycles_per_sample': result['work'], 'samples': result['samples'],
                                  'median_seconds_per_cycle': medians,
                                  'calibration': result['calibration'], 'run_order': result['run_order']})
        destination.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
