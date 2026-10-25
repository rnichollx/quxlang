#!/usr/bin/env python3
"""The runner compares allocator throughput using samples of at least three seconds."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[2]

sys.path.insert(0, str(ROOT.parent))
from measurement import measure_case, minimum_seconds


def digest(path):
    """Identify the exact compiler, source, or executable used in a run."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def host_configuration(config):
    """Returns the native platform and processor supported by the timer."""
    hosts = {('Darwin', 'arm64'): ('macos', 'ARM64'),
             ('Linux', 'aarch64'): ('linux', 'ARM64'),
             ('Linux', 'x86_64'): ('linux', 'x64')}
    host = hosts.get((platform.system(), platform.machine()))
    if host is None:
        raise ValueError('allocator benchmarks support macOS ARM64 and Linux ARM64 or x64')
    return config.replace('platform: macos', 'platform: ' + host[0]).replace('cpu: ARM64', 'cpu: ' + host[1])


def prepare_executables(executables, execute):
    """Applies native macOS metadata when the host requires it."""
    if platform.system() != 'Darwin':
        return None
    sdk = subprocess.check_output(['xcrun', '--show-sdk-version'], text=True).strip()
    for executable in executables.values():
        execute(['xcrun', 'vtool', '-set-build-version', 'macos', '11.0', sdk, '-replace', '-output', executable, executable])
        execute(['codesign', '--force', '--sign', '-', executable])
    return sdk


def counter_description():
    """Returns the native timestamp source recorded by both implementations."""
    return 'CLOCK_MONOTONIC nanoseconds via Linux syscall' if platform.machine() == 'x86_64' else 'CNTVCT_EL0 with ISB'


def add_cpp_build_arguments(parser):
    """The runners expose the CPU tuning and interprocedural optimization used by the C++ baseline."""
    parser.add_argument('--cpp-target', choices=('native', 'generic'), default='native',
                        help='Select CPU tuning for both mimalloc and the matched C++ driver.')
    parser.add_argument('--cpp-lto', choices=('thin', 'none'), default='thin',
                        help='Select ThinLTO or separate compilation for the C++ baseline.')


def cpp_build_flags(args):
    """The helper returns matching C and C++ optimization flags and the required linker flags."""
    common = []
    if args.cpp_target == 'native':
        common.append('-march=native' if platform.machine().lower() in ('x86_64', 'amd64') else '-mcpu=native')
    linker = []
    if args.cpp_lto == 'thin':
        common.append('-flto=thin')
        if platform.system() == 'Linux':
            if shutil.which('ld.lld'):
                linker.append('-fuse-ld=lld')
            elif shutil.which('ld.gold'):
                linker.append('-fuse-ld=gold')
    return common, linker


def main():
    """Build native executables and compare balanced serial repetitions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path, default=REPOSITORY / "tmp/allocator-comparison-out")
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--threads", type=int, nargs="+", default=[1])
    parser.add_argument("--allocators", nargs="+", choices=("quxlang_alloc", "cpp_new", "cpp_mimalloc", "c_malloc", "c_mimalloc"),
                        help="Select measured allocators; defaults to all available variants.")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--mimalloc-source", type=Path, help="Build additional variants from a local mimalloc source checkout.")
    parser.add_argument('--qxc', type=Path, default=REPOSITORY / 'misc/build/buildspaces/system-clang/quxlang/Release/qxc')
    parser.add_argument('--operations', type=int, default=8000000, help='Initial allocations per worker; calibration may increase this count.')
    parser.add_argument('--sizes', type=int, nargs='+', choices=(8, 16, 24, 32, 64), default=[8, 16, 24, 32, 64])
    parser.add_argument('--batches', type=int, nargs='+', choices=range(1, 21), default=[1, 2, 4, 8, 16, 20])
    parser.add_argument("--min-seconds", type=minimum_seconds, default=3.0, help="Minimum seconds per recorded sample (at least 3); calibration trials may be shorter.")
    add_cpp_build_arguments(parser)
    args = parser.parse_args()
    cpp_flags, cpp_linker_flags = cpp_build_flags(args)
    if args.operations < 1 or any(count < 1 or count > 128 for count in args.threads):
        parser.error('operations must be positive and threads must be between 1 and 128')
    if args.repetitions < 1:
        parser.error("--repetitions must be positive")
    if args.allocators and any(name in args.allocators for name in ("cpp_mimalloc", "c_mimalloc")) and not args.mimalloc_source:
        parser.error("mimalloc variants require --mimalloc-source")
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    bundle = work / "bundle"
    standard = REPOSITORY / "quxlang/tests/testdata/testbundle/modules"
    for name in ("std", "runtime", "syscall", "posix"):
        shutil.copytree(standard / name, bundle / "modules" / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / "modules/main", bundle / "modules/main", dirs_exist_ok=True)
    config = (ROOT / "qxcbuild.yml").read_text()
    config = host_configuration(config)
    (bundle / "qxcbuild.yml").write_text(config)
    qxc = args.qxc.resolve()
    commands = [[str(qxc), str(bundle), str(work / "quxlang"), "native"]]
    with (work / "qxc-build.log").open("w") as log:
        subprocess.run(commands[0], stdout=log, stderr=subprocess.STDOUT, check=True)
    executables = {"quxlang_alloc": work / "quxlang/output/allocations"}
    variants = [
        ("cpp_new", "clang++", "allocations.cpp", ["-std=c++17"]),
        ("c_malloc", "clang", "allocations.c", ["-std=c17"]),
    ]
    mimalloc = None
    mimalloc_object = work / "mimalloc.o"
    if args.mimalloc_source:
        mimalloc_source = args.mimalloc_source.resolve()
        command = ["clang", "-std=c17", "-O3", *cpp_flags, "-DNDEBUG", "-DMI_STATIC_LIB",
                   "-I" + str(mimalloc_source / "include"), "-c",
                   str(mimalloc_source / "src/static.c"), "-o", str(mimalloc_object)]
        subprocess.run(command, check=True)
        commands.append(command)
        mimalloc = {"source": str(mimalloc_source),
                    "revision": subprocess.check_output(["git", "-C", str(mimalloc_source), "rev-parse", "HEAD"]).decode().strip(),
                    "object_sha256": digest(mimalloc_object),
                    "source_sha256": {str(path.relative_to(mimalloc_source)): digest(path)
                                      for directory in ("src", "include")
                                      for path in sorted((mimalloc_source / directory).rglob("*"))
                                      if path.is_file()}}
        variants.extend([
            ("cpp_mimalloc", "clang++", "allocations.cpp", ["-std=c++17", "-DUSE_MIMALLOC", "-DMI_STATIC_LIB", "-I" + str(mimalloc_source / "include")]),
            ("c_mimalloc", "clang", "allocations.c", ["-std=c17", "-DUSE_MIMALLOC", "-DMI_STATIC_LIB", "-I" + str(mimalloc_source / "include")]),
        ])
    for name, compiler, source, flags in variants:
        executable = work / name
        # Preserve the requested malloc/free API and prevent calloc substitution.
        language_flags = ['-fsized-deallocation'] if compiler == 'clang++' else []
        command = [compiler, *flags, "-O3", *cpp_flags, *cpp_linker_flags, *language_flags, "-DNDEBUG", "-pthread", "-fno-builtin-malloc", "-fno-builtin-free", str(ROOT / source), str(ROOT / "barriers.S"), "-o", str(executable)]
        if "-DUSE_MIMALLOC" in flags:
            command.append(str(mimalloc_object))
        subprocess.run(command, check=True)
        commands.append(command)
        executables[name] = executable
    if args.allocators:
        executables = {name: path for name, path in executables.items() if name in args.allocators}
    def execute(command):
        """The helper records each native metadata command after successful execution."""
        command = [str(value) for value in command]
        subprocess.run(command, check=True)
        commands.append(command)

    sdk_version = prepare_executables(executables, execute)
    benchmark_environment = os.environ.copy()
    removed_allocator_environment = {name: benchmark_environment.pop(name)
                                     for name in tuple(benchmark_environment)
                                     if name.startswith(("Malloc", "_Malloc", "MIMALLOC_"))}
    report = {"removed_allocator_environment": removed_allocator_environment, "platform": platform.platform(), "commands": commands,
              "cpp_target": args.cpp_target, "cpp_lto": args.cpp_lto,
              "workload": "Allocate the complete batch without payload writes, observe every pointer through an opaque assembly call, then release the complete batch in FIFO order.",
              "counter": counter_description(),
              "sdk_version": sdk_version,
              "unit": "nanoseconds per allocation/free pair",
              "counter_frequency_hz": {},
              "percentage_definition": "100 * Quxlang / min(C++ default allocator, C++ mimalloc)",
              "threads": args.threads[0] if len(args.threads) == 1 else args.threads, "logical_cpus": os.cpu_count(),
              "timing": "One thread measures allocation batches on the main thread. Multiple threads measure start release through last worker completion, excluding creation and teardown, after 10000 warmup allocations per worker.",
              "clang": subprocess.check_output(["clang", "--version"]).decode(),
              "qxc_sha256": digest(qxc), "measurement_sha256": digest(ROOT.parent / "measurement.py"), "repetitions": args.repetitions, "minimum_sample_seconds": args.min_seconds, "mimalloc": mimalloc,
              "binary_sha256": {name: digest(path) for name, path in executables.items()},
              "source_sha256": {str(path.relative_to(REPOSITORY)): digest(path)
                                for directory in (ROOT, standard) for path in sorted(directory.rglob("*"))
                                if path.is_file() and path.suffix in (".qxs", ".c", ".cpp", ".h", ".S", ".py", ".yml")},
              "results": []}
    destination = work / "results.json"
    destination.write_text(json.dumps(report, indent=2) + "\n")
    if args.build_only:
        return
    for threads in args.threads:
        for size in args.sizes:
            for batch in args.batches:
                def execute_sample(name, n):
                    options = [f"--n={n}", f"--size={size}", f"--batch={batch}", f"--threads={threads}"] if name == "quxlang_alloc" else [str(n), str(size), str(batch), str(threads)]
                    output = subprocess.check_output([str(executables[name]), *options], timeout=120, env=benchmark_environment)
                    elapsed, frequency = map(int, output.split())
                    if elapsed <= 0 or frequency <= 0:
                        raise RuntimeError(f"{name}: invalid timing: {output!r}")
                    previous = report["counter_frequency_hz"].setdefault(name, frequency)
                    if previous != frequency or len(set(report["counter_frequency_hz"].values())) != 1:
                        raise RuntimeError("benchmark timer frequencies differ or changed")
                    duration = elapsed / frequency
                    print(f"threads={threads} bytes={size} batch={batch} allocator={name} n={n} seconds={duration:.6f}", flush=True)
                    return {"seconds": duration}

                measurement = measure_case(executables, execute_sample, args.operations, args.repetitions, args.min_seconds)
                n = measurement['work']
                elapsed_samples = {name: [sample['seconds'] for sample in samples] for name, samples in measurement['samples'].items()}
                samples = {name: [duration * 1e9 / (n * threads) for duration in values] for name, values in elapsed_samples.items()}
                medians = {name: statistics.median(values) for name, values in samples.items()}
                report["results"].append({"threads": threads, "bytes": size, "batch": batch, "allocations": n * threads,
                                          "allocations_per_thread": n, "initial_allocations_per_thread": args.operations,
                                          "sample_seconds": elapsed_samples, "calibration": measurement['calibration'],
                                          "ns_per_pair": samples, "median_ns_per_pair": medians})
                destination.write_text(json.dumps(report, indent=2) + "\n")
                print(f"threads={threads} bytes={size} batch={batch}: " + ", ".join(f"{name}={value:.2f} ns/pair" for name, value in medians.items()), flush=True)


if __name__ == "__main__":
    main()
