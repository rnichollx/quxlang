#!/usr/bin/env python3
"""This runner measures existing parallel Quxlang workloads against their serial C++ baselines."""

import argparse
import json
import os
from pathlib import Path
import platform
import random
import shutil
import statistics
import subprocess
import threading
import time

from run import ROOT, REPOSITORY, digest


WORKLOADS = ('fannkuch-redux', 'binary-trees', 'fasta', 'mandelbrot', 'reverse-complement')


def main():
    """Builds parallel workloads and validates every measured output against C++."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qxc', type=Path, required=True)
    parser.add_argument('--baseline-dir', type=Path, required=True)
    parser.add_argument('--work-dir', type=Path, default=REPOSITORY / 'tmp/benchmarksgame-scaling')
    parser.add_argument('--threads', type=int, nargs='+', default=[1, 2, 4, 8, 16, 32, 64, 128])
    parser.add_argument('--repetitions', type=int, default=5)
    parser.add_argument('--timeout', type=int, default=600)
    options = parser.parse_args()
    if options.repetitions < 1 or any(count < 1 or count > 128 for count in options.threads):
        parser.error('repetitions must be positive and threads must be between 1 and 128')
    work = options.work_dir.resolve()
    baseline = options.baseline_dir.resolve()
    bundle = work / 'bundle'
    standard = REPOSITORY / 'quxlang/tests/testdata/testbundle/modules'
    for name in ('runtime', 'std', 'syscall', 'posix'):
        shutil.copytree(standard / name, bundle / 'modules' / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / 'modules/game', bundle / 'modules/game', dirs_exist_ok=True)
    config = (ROOT / 'qxcbuild.yml').read_text().split('outputs:')[0] + 'outputs:\n'
    config = config.replace('platform: macos', 'platform: ' + {'Linux': 'linux', 'Darwin': 'macos'}[platform.system()])
    config = config.replace('cpu: ARM64', 'cpu: ' + {'x86_64': 'x64', 'arm64': 'ARM64', 'aarch64': 'ARM64'}[platform.machine()])
    for name in WORKLOADS:
        config += (f'  {name}-fast-mt:\n    target: native\n    type: benchmark_executable\n'
                   f'    build_type: Release\n    benchmark: "::{name.replace("-", "_")}_fast_mt"\n')
    (bundle / 'qxcbuild.yml').write_text(config)
    command = [str(options.qxc.resolve()), str(bundle), str(work / 'quxlang'), 'native']
    with (work / 'qxc-build.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    environment = {key: value for key, value in os.environ.items() if not key.startswith(('Malloc', '_Malloc', 'MIMALLOC_'))}
    workloads = json.loads((ROOT / 'workloads.json').read_text())
    report = {'platform': platform.platform(), 'logical_cpus': os.cpu_count(),
              'comparison': 'Parallel Quxlang versus serial C++; C++ has no parallel implementation in this suite.',
              'repetitions': options.repetitions, 'discarded_warmup_runs': 1,
              'qxc_sha256': digest(options.qxc), 'build_command': command,
              'source_sha256': {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob('*')) if path.is_file()},
              'results': []}
    randomizer = random.Random(20261003)
    for name in WORKLOADS:
        workload = workloads[name]
        n = workload['performance_n']
        executable = work / 'quxlang/output' / (name + '-fast-mt')
        cpp = baseline / 'cpp' / name
        input_path = baseline / f'fasta-input-{n}.txt' if workload.get('stdin') else None
        for count in options.threads:
            invocation = [str(executable), f'--threads={count}'] + ([] if input_path else [f'--n={workload["verify_n"]}'])
            source = (ROOT / 'fixtures/fasta-1000.txt').open('rb') if input_path else None
            try:
                actual = subprocess.check_output(invocation, stdin=source, env=environment, timeout=options.timeout)
            finally:
                if source:
                    source.close()
            expected = ((ROOT / 'fixtures' / workload['expected_file']).read_bytes()
                        if 'expected_file' in workload else workload['expected'].encode())
            if actual != expected:
                raise RuntimeError(f'{name} threads={count}: reference output differs')
        expected = work / (name + '-reference.out')
        with expected.open('wb') as output:
            source = input_path.open('rb') if input_path else None
            try:
                subprocess.run([str(cpp)] + ([] if input_path else [str(n)]), stdin=source, stdout=output,
                               env=environment, check=True, timeout=options.timeout)
            finally:
                if source:
                    source.close()
        expected_hash = digest(expected)
        samples = {count: [] for count in options.threads}
        orders = []
        for repetition in range(options.repetitions + 1):
            order = list(options.threads)
            randomizer.shuffle(order)
            orders.append(order)
            for count in order:
                invocation = [str(executable), f'--threads={count}'] + ([] if input_path else [f'--n={n}'])
                actual = work / (name + '-actual.out')
                source = input_path.open('rb') if input_path else None
                try:
                    with actual.open('wb') as output:
                        start = time.perf_counter_ns()
                        process = subprocess.Popen(invocation, stdin=source, stdout=output, env=environment)
                        watchdog = threading.Timer(options.timeout, process.kill)
                        watchdog.start()
                        status = process.wait()
                        elapsed = (time.perf_counter_ns() - start) / 1e9
                        watchdog.cancel()
                        watchdog.join()
                        if status != 0:
                            raise subprocess.CalledProcessError(status, invocation)
                finally:
                    if source:
                        source.close()
                if digest(actual) != expected_hash:
                    raise RuntimeError(f'{name} threads={count}: output differs from C++')
                if repetition:
                    samples[count].append(elapsed)
                print(f'{name} threads={count} run={repetition}: {elapsed:.6f}s, output verified', flush=True)
        medians = {count: statistics.median(values) for count, values in samples.items()}
        report['results'].append({'workload': name, 'n': n, 'seconds': samples, 'median_seconds': medians,
                                  'run_order': orders, 'output_sha256': expected_hash,
                                  'binary_sha256': digest(executable), 'cpp_binary_sha256': digest(cpp)})
        (work / 'results.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
