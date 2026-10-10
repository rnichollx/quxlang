#!/usr/bin/env python3
"""Compare default allocators with uninitialized storage and opaque assembly barriers."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import shutil
import statistics
import subprocess


ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[2]


def digest(path):
    """Identify the exact compiler, source, or executable used in a run."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    """Build native executables and compare balanced serial repetitions."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path, default=REPOSITORY / "allocator-comparison-out")
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--threads", type=int, choices=(1, 32), default=1)
    parser.add_argument("--allocators", nargs="+", choices=("quxlang_alloc", "cpp_new", "cpp_mimalloc", "c_malloc", "c_mimalloc"),
                        help="Select measured allocators; defaults to all available variants.")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--mimalloc-source", type=Path, help="Build additional variants from a local mimalloc source checkout.")
    args = parser.parse_args()
    if platform.machine() not in ("arm64", "aarch64"):
        parser.error("the allocator benchmark timer requires ARM64")
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
    config = config.replace("platform: macos", "platform: " + {"Darwin": "macos", "Linux": "linux"}[platform.system()])
    config = config.replace("cpu: ARM64", "cpu: " + {"arm64": "ARM64", "aarch64": "ARM64"}[platform.machine()])
    (bundle / "qxcbuild.yml").write_text(config)
    qxc = REPOSITORY / "misc/build/buildspaces/system-clang/quxlang/Release/qxc"
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
        command = ["clang", "-std=c17", "-O3", "-DNDEBUG", "-DMI_STATIC_LIB",
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
        command = [compiler, *flags, "-O3", "-DNDEBUG", "-pthread", "-fno-builtin-malloc", "-fno-builtin-free", str(ROOT / source), str(ROOT / "barriers.S"), "-o", str(executable)]
        if "-DUSE_MIMALLOC" in flags:
            command.append(str(mimalloc_object))
        subprocess.run(command, check=True)
        commands.append(command)
        executables[name] = executable
    if args.allocators:
        executables = {name: path for name, path in executables.items() if name in args.allocators}
    sdk_version = None
    if platform.system() == "Darwin":
        sdk_version = subprocess.check_output(["xcrun", "--show-sdk-version"], text=True).strip()
        # Modern SDK metadata enables the highest architectural counter frequency.
        for executable in executables.values():
            command = ["xcrun", "vtool", "-set-build-version", "macos", "11.0", sdk_version,
                       "-replace", "-output", str(executable), str(executable)]
            subprocess.run(command, check=True)
            commands.append(command)
            command = ["codesign", "--force", "--sign", "-", str(executable)]
            subprocess.run(command, check=True)
            commands.append(command)
    benchmark_environment = os.environ.copy()
    removed_allocator_environment = {name: benchmark_environment.pop(name)
                                     for name in tuple(benchmark_environment)
                                     if name.startswith(("Malloc", "_Malloc", "MIMALLOC_"))}
    report = {"removed_allocator_environment": removed_allocator_environment, "platform": platform.platform(), "commands": commands,
              "workload": "Allocate the complete batch without payload writes, observe every pointer through an opaque assembly call, then release the complete batch in FIFO order.",
              "counter": "CNTVCT_EL0 with ISB",
              "sdk_version": sdk_version,
              "unit": "nanoseconds per allocation/free pair",
              "counter_frequency_hz": {},
              "percentage_definition": "100 * Quxlang / min(C++ default allocator, C++ mimalloc)",
              "threads": args.threads, "logical_cpus": os.cpu_count(),
              "timing": "Wall time from start release through last worker completion; creation and teardown excluded; 10000 warmup allocations per worker." if args.threads == 32 else "Wall time around allocation batches on the main thread.",
              "clang": subprocess.check_output(["clang", "--version"]).decode(),
              "qxc_sha256": digest(qxc), "repetitions": args.repetitions, "mimalloc": mimalloc,
              "binary_sha256": {name: digest(path) for name, path in executables.items()},
              "source_sha256": {str(path.relative_to(REPOSITORY)): digest(path)
                                for directory in (ROOT, standard) for path in sorted(directory.rglob("*"))
                                if path.is_file() and path.suffix in (".qxs", ".c", ".cpp", ".h", ".S", ".py", ".yml")},
              "results": []}
    destination = work / "results.json"
    destination.write_text(json.dumps(report, indent=2) + "\n")
    if args.build_only:
        return
    randomizer = random.Random(20261003)
    for size in (8, 16, 24, 32, 64):
        n = 8000000
        for batch in (1, 2, 4, 8, 16, 20):
            samples = {name: [] for name in executables}
            order = list(executables)
            randomizer.shuffle(order)
            for repeat in range(args.repetitions + 1):
                for name in order[repeat % len(order):] + order[:repeat % len(order)]:
                    options = [f"--n={n}", f"--size={size}", f"--batch={batch}", f"--threads={args.threads}"] if name == "quxlang_alloc" else [str(n), str(size), str(batch), str(args.threads)]
                    output = subprocess.check_output([str(executables[name]), *options], timeout=120, env=benchmark_environment)
                    elapsed, frequency = map(int, output.split())
                    if elapsed <= 0 or frequency <= 0:
                        raise RuntimeError(f"{name}: invalid timing: {output!r}")
                    if name not in report["counter_frequency_hz"]:
                        report["counter_frequency_hz"][name] = frequency
                    elif report["counter_frequency_hz"][name] != frequency:
                        raise RuntimeError(f"{name}: timer frequency changed between runs")
                    if len(set(report["counter_frequency_hz"].values())) != 1:
                        raise RuntimeError("benchmark executables use different counter frequencies")
                    if repeat:
                        samples[name].append(elapsed * 1000000000 / (frequency * n * args.threads))
            medians = {name: statistics.median(values) for name, values in samples.items()}
            report["results"].append({"bytes": size, "batch": batch, "allocations": n * args.threads,
                                      "allocations_per_thread": n,
                                      "ns_per_pair": samples, "median_ns_per_pair": medians})
            destination.write_text(json.dumps(report, indent=2) + "\n")
            print(f"bytes={size} batch={batch}: " + ", ".join(f"{name}={value:.2f} ns/pair" for name, value in medians.items()), flush=True)


if __name__ == "__main__":
    main()
