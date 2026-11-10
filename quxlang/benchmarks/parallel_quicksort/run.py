#!/usr/bin/env python3
"""Measure Quxlang std::thread and GO quicksort with verified Release samples."""

import argparse
from functools import lru_cache
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess


ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[2]
MASK = (1 << 64) - 1


def mix(value):
    """Generate the independent reference input."""
    value = (value + 11400714819323198485) & MASK
    value = ((value ^ (value >> 30)) * 13787848793156543929) & MASK
    value = ((value ^ (value >> 27)) * 10723151780598845931) & MASK
    return value ^ (value >> 31)


@lru_cache(maxsize=None)
def seed_checksums(size):
    """Calculate one sorted checksum for each of the eight input seeds."""
    checksums = []
    for offset in range(8):
        values = sorted(mix(20261108 + offset + index) for index in range(size))
        checksums.append(sum(value * (index + 1) for index, value in enumerate(values)) & MASK)
    return checksums


def expected_checksum(size, passes):
    """Calculate the complete expected checksum independently of both variants."""
    checksums = seed_checksums(size)
    return (sum(checksums) * (passes // 8) + sum(checksums[:passes % 8])) & MASK


def digest(path):
    """Identify a source or executable artifact."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(args):
    """Assemble the checked-in stdlib and compile both Quxlang benchmarks."""
    bundle = args.work_dir / "bundle"
    standard = REPOSITORY / "quxlang/tests/testdata/testbundle/modules"
    for name in ("runtime", "std", "syscall", "posix", "io"):
        shutil.copytree(standard / name, bundle / "modules" / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / "modules/main", bundle / "modules/main", dirs_exist_ok=True)
    host_os = {"Darwin": "macos", "Linux": "linux", "Windows": "windows"}[platform.system()]
    host_cpu = {"arm64": "ARM64", "aarch64": "ARM64", "x86_64": "x64", "AMD64": "x64"}[platform.machine()]
    config = (ROOT / "qxcbuild.yml").read_text()
    config = config.replace("platform: macos", "platform: " + host_os).replace("cpu: ARM64", "cpu: " + host_cpu)
    (bundle / "qxcbuild.yml").write_text(config)
    command = [str(args.qxc), str(bundle), str(args.work_dir / "quxlang"), "native"]
    with (args.work_dir / "build.log").open("w") as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    return command


def run_case(binary, arguments, expected):
    """Run one variant and validate its complete sorted output."""
    output = subprocess.check_output([str(binary), *arguments], text=True, timeout=600).split()
    if len(output) != 2:
        raise RuntimeError(f"Unexpected benchmark output: {output!r}")
    elapsed, checksum = map(int, output)
    if elapsed <= 0 or checksum != expected:
        raise RuntimeError(f"Invalid benchmark result: {output!r}; expected {expected}")
    return elapsed


def main():
    """Calibrate a common pass count and record interleaved samples."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--cutoff", type=int, default=2)
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--workers", type=int, default=os.cpu_count())
    parser.add_argument("--work-dir", type=Path, default=REPOSITORY / "tmp/parallel-quicksort")
    parser.add_argument("--qxc", type=Path, default=REPOSITORY / "misc/build/buildspaces/system-clang/quxlang/Release/qxc")
    args = parser.parse_args()
    if args.size < 2 or args.size > 4194304 or args.cutoff < 2 or args.repetitions < 1 or not args.workers:
        parser.error("invalid size, cutoff, repetition count, or worker count")
    args.work_dir = args.work_dir.resolve()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    command = build(args)
    binaries = {"threads": args.work_dir / "quxlang/output/quicksort_threads",
                "go": args.work_dir / "quxlang/output/quicksort_go"}

    def measure(name, passes):
        arguments = [f"--n={args.size}", f"--cutoff={args.cutoff}", f"--passes={passes}"]
        if name == "go":
            arguments.append(f"--workers={args.workers}")
        return run_case(binaries[name], arguments, expected_checksum(args.size, passes))

    for name in binaries:
        measure(name, 8)
    passes = 8
    while max(measure(name, passes) for name in binaries) < 500_000_000:
        passes *= 2
    samples = {name: [] for name in binaries}
    for repetition in range(args.repetitions):
        order = ("threads", "go") if repetition % 2 == 0 else ("go", "threads")
        for name in order:
            nanoseconds = measure(name, passes)
            samples[name].append(nanoseconds / passes)
            print(f"{name} sample {repetition + 1}: {nanoseconds / passes / 1e6:.3f} ms/sort", flush=True)
    median = {name: statistics.median(values) for name, values in samples.items()}
    report = {
        "platform": platform.platform(), "architecture": platform.machine(),
        "size": args.size, "cutoff": args.cutoff, "passes_per_sample": passes,
        "hardware_concurrency": os.cpu_count(), "pool_workers": args.workers,
        "steppings": "DEFAULT (no overrides)", "build_command": command,
        "qxc_sha256": digest(args.qxc),
        "source_sha256": digest(ROOT / "modules/main/sources/quicksort.qxs"),
        "binary_sha256": {name: digest(path) for name, path in binaries.items()},
        "nanoseconds_per_sort": samples, "median_nanoseconds_per_sort": median,
        "go_over_threads": median["go"] / median["threads"],
    }
    (args.work_dir / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"median: threads {median['threads'] / 1e6:.3f} ms, GO {median['go'] / 1e6:.3f} ms; "
          f"GO/threads {report['go_over_threads']:.3f}")


if __name__ == "__main__":
    main()
