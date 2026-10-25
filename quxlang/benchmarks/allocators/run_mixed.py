#!/usr/bin/env python3
"""The runner compares repeated MT19937 allocation workloads with bounded input storage."""

import argparse
import array
import sys
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess

from run import add_cpp_build_arguments, cpp_build_flags, ROOT, REPOSITORY, digest, host_configuration, prepare_executables, counter_description, measure_case, minimum_seconds


def main():
    """Build Release binaries and measure repeated runs after one discarded warmup."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'tmp/allocator-comparison-out/mixed')
    parser.add_argument('--mimalloc-source', type=Path, required=True)
    parser.add_argument('--operations', type=int, default=8000000, help='Pre-generated operations per worker in one sequence pass.')
    parser.add_argument('--slots', type=int, nargs='+', default=[4096, 65536])
    parser.add_argument('--threads', type=int, nargs='+', default=[1])
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--workload', choices=['private', 'atomic-shared'], default='private',
                        help='Private allocation toggles or shared atomic replacements.')
    parser.add_argument('--qxc', type=Path, default=REPOSITORY / 'misc/build/buildspaces/system-clang/quxlang/Release/qxc')
    parser.add_argument("--min-seconds", type=minimum_seconds, default=3.0, help="Minimum seconds per recorded sample (at least 3); calibration trials may be shorter.")
    add_cpp_build_arguments(parser)
    args = parser.parse_args()
    cpp_flags, cpp_linker_flags = cpp_build_flags(args)
    if args.operations < 1:
        parser.error('--operations must be positive')
    if args.repetitions < 1:
        parser.error('--repetitions must be positive')
    if any(slots < 1 or slots > 2**29 for slots in args.slots):
        parser.error('--slots must be between 1 and 536870912')
    if any(count < 1 for count in args.threads):
        parser.error('--threads must be positive')
    if args.workload == 'private' and max(args.threads) > min(args.slots):
        parser.error('private workloads require threads no greater than slots')
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('std', 'runtime', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/main', bundle / 'modules/main', dirs_exist_ok=True)
    benchmark = 'atomic_mixed_allocations' if args.workload == 'atomic-shared' else 'mixed_allocations'
    cpp_source = ROOT / (benchmark + '.cpp')
    config = (ROOT / 'qxcbuild.yml').read_text().replace('benchmark: "::allocations"', f'benchmark: "::{benchmark}"')
    (bundle / 'qxcbuild.yml').write_text(host_configuration(config))
    qxc = args.qxc.resolve()
    commands = []

    def execute(command, **kwargs):
        """Record and execute a build or input-generation command."""
        commands.append([str(value) for value in command])
        subprocess.run(command, check=True, **kwargs)

    with (work / 'qxc-build.log').open('w') as log:
        execute([qxc, bundle, work / 'quxlang', 'native'], stdout=log, stderr=subprocess.STDOUT)
    mimalloc = args.mimalloc_source.resolve()
    execute(['clang', '-std=c17', '-O3', *cpp_flags, '-DNDEBUG', '-DMI_STATIC_LIB', '-I' + str(mimalloc / 'include'),
             '-c', mimalloc / 'src/static.c', '-o', work / 'mimalloc.o'])
    execute(['clang++', '-std=c++17', '-O3', *cpp_flags, *cpp_linker_flags, '-fsized-deallocation', '-DNDEBUG', '-DMI_STATIC_LIB', '-pthread',
             '-fno-builtin-malloc', '-fno-builtin-free', '-I' + str(mimalloc / 'include'),
             cpp_source, ROOT / 'barriers.S', work / 'mimalloc.o', '-o', work / 'cpp_mimalloc'])
    executables = {'quxlang_alloc': work / 'quxlang/output/allocations', 'cpp_mimalloc': work / 'cpp_mimalloc'}
    sdk = prepare_executables(executables, execute)
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    report = {'platform': platform.platform(), 'cpp_target': args.cpp_target, 'cpp_lto': args.cpp_lto, 'commands': commands, 'sequence_operations_per_thread': args.operations, 'minimum_sample_seconds': args.min_seconds,
              'measurement_sha256': digest(ROOT.parent / 'measurement.py'),
              'pass_protocol': 'Workers replay the bounded input without reinitializing slots or recreating threads. Odd pass counts preserve private-slot reference results.',
              'sizes': [8, 16, 24, 32, 48, 64, 128, 256], 'seed': 20261009, 'generator': 'std::mt19937',
              'sequences': {}, 'selection': 'size = word % 8; slot = (word / 8) % worker_slots',
              'partition': 'Only slots are divided: floor(K / T), with one extra for the first K % T workers; every worker executes N operations',
              'worker_seed': '20261009 + worker_index', 'sequence_checksum_algorithm': 'XOR of all U32 words',
              'unit': 'ns per allocation or deallocation operation', 'repetitions': args.repetitions, 'discarded_warmup_runs': 1,
              'payload_write': 'Store byte value 1 at offset zero immediately after every allocation using matching opaque assembly calls.',
              'timing': 'Thread creation through all joins, including worker TLS teardown; input loading, null initialization and final cleanup excluded.',
              'counter': counter_description(), 'sdk_version': sdk,
              'qxc_sha256': digest(qxc), 'build_config_sha256': digest(bundle / 'qxcbuild.yml'),
              'binary_sha256': {name: digest(path) for name, path in executables.items()},
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*.qxs'))},
              'cpp_source_sha256': digest(cpp_source), 'barriers_sha256': digest(ROOT / 'barriers.S'),
              'mimalloc_revision': subprocess.check_output(['git', '-C', mimalloc, 'rev-parse', 'HEAD'], text=True).strip(),
              'results': []}
    report['workload'] = args.workload
    if args.workload == 'atomic-shared':
        report['selection'] = 'size = word % 8; slot = (word / 8) % shared_slots'
        report['partition'] = 'One shared array per size; every worker accesses all K slots and executes N replacements.'
        report['atomic_swap'] = 'Strong compare-exchange retry loop; acquire-release success, relaxed failure and initial load.'
        report['unit'] = 'ns per allocate/swap/conditional-free replacement'
    for threads in args.threads:
        sequence = work / f'mt19937-t{threads}.bin'
        with sequence.open('wb') as output:
            execute([executables['cpp_mimalloc'], str(args.operations), '0', str(threads)], stdout=output)
        sequence_operations = args.operations * threads
        assert sequence.stat().st_size == sequence_operations * 4
        sequence_hash = hashlib.sha256()
        with sequence.open('rb') as source:
            for chunk in iter(lambda: source.read(8 * 1024 * 1024), b''):
                sequence_hash.update(chunk)
        report['sequences'][str(threads)] = {'path': str(sequence), 'sha256': sequence_hash.hexdigest()}
        for slots in args.slots:
            reference = None
            if args.workload == 'atomic-shared':
                occupied = [bytearray(slots) for size in report['sizes']]
                checksum = 0
                with sequence.open('rb') as source:
                    while True:
                        block = source.read(4 * 1024 * 1024)
                        if not block:
                            break
                        words = array.array('I')
                        words.frombytes(block)
                        if sys.byteorder != 'little':
                            words.byteswap()
                        for word in words:
                            checksum ^= word
                            occupied[word % 8][(word // 8) % slots] = 1
                counts = [sum(cells) for cells in occupied]
                reference = (checksum, sum(counts), sum(count * size for count, size in zip(counts, report['sizes'])))
            validation = None
            frequencies = set()

            def execute_sample(name, passes):
                nonlocal validation
                options = [f'--n={args.operations}', f'--slots={slots}', f'--threads={threads}', f'--passes={passes}'] if name == 'quxlang_alloc' else [str(args.operations), str(slots), str(threads), str(passes)]
                with sequence.open('rb') as source:
                    output = subprocess.check_output([executables[name], *options], stdin=source, env=environment, timeout=1200)
                ticks, frequency, checksum, live, live_bytes = map(int, output.split())
                if ticks <= 0 or frequency <= 0:
                    raise RuntimeError(f'{name}: invalid timer: {output!r}')
                frequencies.add(frequency)
                if len(frequencies) != 1:
                    raise RuntimeError('benchmark timer frequencies differ or changed')
                actual = (checksum, live, live_bytes)
                if reference is not None and actual != reference:
                    raise RuntimeError(f'{name}: independent reference mismatch: {actual} != {reference}')
                if validation is None:
                    validation = actual
                if actual != validation:
                    raise RuntimeError(f'{name}: implementation output differs: {actual} != {validation}')
                duration = ticks / frequency
                print(f'T={threads} K={slots} allocator={name} passes={passes} seconds={duration:.6f}', flush=True)
                return {'seconds': duration, 'frequency_hz': frequency}

            measurement = measure_case(executables, execute_sample, 1, args.repetitions, args.min_seconds, odd_work=True)
            passes = measurement['work']
            total_operations = sequence_operations * passes
            elapsed_samples = {name: [sample['seconds'] for sample in values] for name, values in measurement['samples'].items()}
            samples = {name: [duration * 1e9 / total_operations for duration in values] for name, values in elapsed_samples.items()}
            frequency = next(iter(frequencies))
            medians = {name: statistics.median(values) for name, values in samples.items()}
            elapsed_seconds = {name: value * total_operations / 1e9 for name, value in medians.items()}
            row = {'threads': threads, 'total_operations': total_operations,
                   'passes': passes, 'operations_per_thread': args.operations * passes,
                   'sample_seconds': elapsed_samples, 'calibration': measurement['calibration'], 'slots_per_size': slots, 'samples_ns_per_operation': samples, 'median_ns_per_operation': medians,
                   'median_elapsed_seconds': elapsed_seconds, 'counter_frequency_hz': frequency, 'sequence_checksum': validation[0], 'remaining_live_allocations': validation[1],
                   'remaining_live_bytes': validation[2], 'independent_reference': reference,
                   'percentage': 100 * medians['quxlang_alloc'] / medians['cpp_mimalloc']}
            report['results'].append(row)
            (work / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
            print(json.dumps(row), flush=True)

if __name__ == '__main__':
    main()
