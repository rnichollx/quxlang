#!/usr/bin/env python3
"""Build and compare direct nested coroutine implementations using verified, repeated Release samples."""

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

MASK = (1 << 64) - 1
ALGORITHMS = {"quicksort": 0, "reduction": 1, "queens": 2}


def digest(path):
    """Identify a source, compiler, or executable artifact."""
    return hashlib.sha256(path.read_bytes()).hexdigest()


def mix(value):
    """Compute the independent unsigned input generator."""
    value = (value + 11400714819323198485) & MASK
    value = ((value ^ (value >> 30)) * 13787848793156543929) & MASK
    value = ((value ^ (value >> 27)) * 10723151780598845931) & MASK
    return value ^ (value >> 31)


def reference(algorithm, n):
    """Return one expected checksum for each of the eight repeating input seeds."""
    if algorithm == "queens":
        # Exact solution counts for complete boards, including asymmetric solutions.
        return [0, 1, 0, 0, 2, 10, 4, 40, 92, 352, 724, 2680, 14200, 73712, 365596][n:n+1] * 8
    checksums = []
    for offset in range(8):
        values = [mix(20261104 + offset + index) for index in range(n)]
        if algorithm == "quicksort":
            checksums.append(sum(value * (index + 1) for index, value in enumerate(sorted(values))) & MASK)
        else:
            checksums.append(sum(values) & MASK)
    return checksums


def task_count(algorithm, n, cutoff):
    """Count coroutine invocations independently without counters in timed execution."""
    if algorithm == "reduction":
        def count(size):
            return 1 if size <= cutoff else 1 + count(size // 2) + count(size - size // 2)
        return [count(n)] * 8
    if algorithm == "queens":
        mask = (1 << n) - 1
        def count(columns, left, right, depth):
            if not depth or columns == mask:
                return 1
            available = mask & ~(columns | left | right)
            total = 1
            while available:
                bit = available & -available
                available -= bit
                total += count(columns | bit, (left | bit) << 1, (right | bit) >> 1, depth - 1)
            return total
        return [count(0, 0, 0, cutoff)] * 8
    counts = []
    for offset in range(8):
        values = [mix(20261104 + offset + index) for index in range(n)]
        def count(first, last):
            if last - first <= cutoff:
                return 1
            middle = first + (last - first) // 2
            values[middle], values[last - 1] = values[last - 1], values[middle]
            pivot, destination = values[last - 1], first
            for index in range(first, last - 1):
                if values[index] < pivot:
                    values[destination], values[index] = values[index], values[destination]
                    destination += 1
            values[destination], values[last - 1] = values[last - 1], values[destination]
            return 1 + count(first, destination) + count(destination + 1, last)
        counts.append(count(0, n))
    return counts


def build(args):
    """Compile the existing Quxlang runtime and a C++20 coroutine comparison."""
    work = args.work_dir
    bundle = work / "bundle"
    standard = REPOSITORY / "quxlang/tests/testdata/testbundle/modules"
    for name in ("runtime", "std", "syscall", "posix", "io"):
        shutil.copytree(standard / name, bundle / "modules" / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / "modules/main", bundle / "modules/main", dirs_exist_ok=True)
    config = (ROOT / "qxcbuild.yml").read_text()
    host_os = {"Darwin": "macos", "Linux": "linux", "Windows": "windows"}[platform.system()]
    host_cpu = {"arm64": "ARM64", "aarch64": "ARM64", "x86_64": "x64", "AMD64": "x64"}[platform.machine()]
    config = config.replace("platform: macos", "platform: " + host_os).replace("cpu: ARM64", "cpu: " + host_cpu)
    (bundle / "qxcbuild.yml").write_text(config)
    commands = [[str(args.qxc), str(bundle), str(work / "quxlang"), "native"],
                [args.cxx, "-std=c++20", "-O3", "-DNDEBUG", str(ROOT / "algorithms.cpp"), "-o", str(work / "cpp")]]
    for index, command in enumerate(commands):
        with (work / f"build-{index}.log").open("w") as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    manifest = {"commands": commands, "qxc_sha256": digest(args.qxc),
                "cpp_compiler": subprocess.check_output([args.cxx, "--version"], text=True),
                "bundle_config_sha256": digest(bundle / "qxcbuild.yml"),
                "bundle_sources_sha256": {str(path.relative_to(bundle)): digest(path) for path in sorted(bundle.rglob("*.qxs"))},
                "cpp_sources_sha256": {name: digest(ROOT / name) for name in ("algorithms.cpp", "runtime.hpp")}}
    (work / "build.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def summarize(report, destination):
    """Report coroutine time and estimated overhead per invocation against ordinary calls."""
    rows = report["results"]
    lines = ["# Direct nested coroutine results", "",
             "Each recursive call is a coroutine down to the configured leaf. All execution uses the calling thread. Allocation, invocation, completion and destruction are included.", "",
             "Delta is the coroutine time minus its language's ordinary-function baseline, divided by the number of coroutine invocations. It is an aggregate overhead estimate, including allocation and optimization differences.", "",
             "| Algorithm | Size | Leaf/depth | Mode | Calls/input | C++ ns/call | Quxlang ns/call | Quxlang / C++ | C++ delta ns/call | Quxlang delta ns/call |",
             "| --- | ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: |"]
    for row in rows:
        baseline = next(item for item in rows if item["algorithm"] == row["algorithm"] and item["n"] == row["n"]
                        and item["mode"] == "ordinary" and item["cutoff"] == row["cutoff"])
        times = row["median_seconds_per_input"]
        count = sum(row["calls_per_seed"]) / 8
        per_call = {name: value * 1e9 / count for name, value in times.items()}
        delta = {name: (times[name] - baseline["median_seconds_per_input"][name]) * 1e9 / count for name in times}
        lines.append(f"| {row['algorithm']} | {row['n']} | {row['cutoff']} | {row['mode']} | {count:.1f} | "
                     f"{per_call['cpp']:.2f} | {per_call['quxlang']:.2f} | {row['quxlang_over_cpp']:.3f} | "
                     f"{delta['cpp']:.2f} | {delta['quxlang']:.2f} |")
    destination.write_text("\n".join(lines) + "\n")


def main():
    """Validate outputs, calibrate common pass counts, and record interleaved samples."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work-dir", type=Path, default=REPOSITORY / "tmp/coroutine-direct")
    parser.add_argument("--qxc", type=Path, default=REPOSITORY / "misc/build/buildspaces/system-clang/quxlang/Release/qxc")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--build-only", action="store_true")
    parser.add_argument("--skip-build", action="store_true")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--algorithms", nargs="+", choices=ALGORITHMS, default=list(ALGORITHMS))
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--cutoffs", type=int, nargs="+", default=[1])
    parser.add_argument("--queens-size", type=int, default=8)
    parser.add_argument("--queens-depths", type=int, nargs="+", default=None)
    parser.add_argument("--repetitions", type=int, default=3)
    parser.add_argument("--min-seconds", type=minimum_seconds, default=3.0)
    args = parser.parse_args()
    if not 1 <= args.size <= 4194304 or not 1 <= args.queens_size <= 14 or args.repetitions < 1:
        parser.error("invalid size or repetition count")
    if any(cutoff < 1 for cutoff in args.cutoffs):
        parser.error("cutoffs must be positive")
    if args.queens_depths is None:
        args.queens_depths = [args.queens_size]
    if any(not 1 <= depth <= args.queens_size for depth in args.queens_depths):
        parser.error("queen depths must be 1..board size")
    if args.build_only and args.skip_build:
        parser.error("--build-only and --skip-build are mutually exclusive")
    args.work_dir = args.work_dir.resolve()
    args.work_dir.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((args.work_dir / "build.json").read_text()) if args.skip_build else build(args)
    if args.build_only:
        return
    if args.skip_build:
        if digest(args.qxc) != manifest["qxc_sha256"]:
            raise RuntimeError("qxc changed since the recorded build")
        for name, expected_hash in manifest["cpp_sources_sha256"].items():
            if digest(ROOT / name) != expected_hash:
                raise RuntimeError(f"{name} changed since the recorded build")
        main_source = "modules/main/sources/algorithms.qxs"
        if digest(ROOT / main_source) != manifest["bundle_sources_sha256"][main_source]:
            raise RuntimeError("Quxlang benchmark source changed since the recorded build")
    binaries = {"cpp": args.work_dir / "cpp", "quxlang": args.work_dir / "quxlang/output/algorithms"}
    report = {"platform": platform.platform(), "architecture": platform.machine(), "logical_cpus": os.cpu_count(), "compiler": subprocess.check_output([args.cxx, "--version"], text=True),
              "qxc_sha256": digest(args.qxc), "binary_sha256": {name: digest(path) for name, path in binaries.items()},
              "source_sha256": {str(path.relative_to(ROOT)): digest(path) for path in ROOT.rglob("*") if path.is_file() and "__pycache__" not in path.parts},
              "build": manifest, "measurement_sha256": digest(ROOT.parent / "measurement.py"),
              "steppings": "DEFAULT (no overrides)", "driver": "direct calling thread", "inputs_per_round": 8, "verification": [], "results": []}
    for algorithm in args.algorithms:
        n = args.queens_size if algorithm == "queens" else args.size
        cutoffs = args.queens_depths if algorithm == "queens" else args.cutoffs
        expected = reference(algorithm, n)
        cases = [(mode, cutoff) for cutoff in cutoffs for mode in range(3)]
        for mode, cutoff in cases:
            def execute(language, rounds):
                passes = rounds * 8
                numbers = [ALGORITHMS[algorithm], n, cutoff, mode, passes]
                options = [f"--{key}={value}" for key, value in zip(("algorithm", "n", "cutoff", "mode", "passes"), numbers)] if language == "quxlang" else list(map(str, numbers))
                output = subprocess.check_output([str(binaries[language]), *options], text=True, timeout=600)
                elapsed, checksum = map(int, output.split())
                correct = (sum(expected) * (passes // 8) + sum(expected[:passes % 8])) & MASK
                if checksum != correct or elapsed <= 0:
                    raise RuntimeError(f"{algorithm}/{language}: invalid result {output!r}; expected checksum {correct}")
                return {"seconds": elapsed / 1e9, "checksum": checksum}
            verification = {language: execute(language, 1) for language in binaries}
            report["verification"].append({"algorithm": algorithm, "n": n, "cutoff": cutoff,
                                           "mode": mode, "passes": 8, "expected_checksums": expected, "runs": verification})
            print(f"verified {algorithm} mode={mode} cutoff={cutoff}", flush=True)
            if args.verify_only:
                continue
            measurement = measure_case(binaries, execute, 1, args.repetitions, args.min_seconds)
            passes = measurement["work"] * 8
            medians = {language: statistics.median(sample["seconds"] / passes for sample in samples)
                       for language, samples in measurement["samples"].items()}
            row = {"algorithm": algorithm, "n": n, "mode": ("ordinary", "coroutine", "suspend")[mode],
                   "cutoff": cutoff, "calls_per_seed": task_count(algorithm, n, cutoff), "passes_per_sample": passes,
                   "median_seconds_per_input": medians, "quxlang_over_cpp": medians["quxlang"] / medians["cpp"], **measurement}
            report["results"].append(row)
            (args.work_dir / "results.json").write_text(json.dumps(report, indent=2) + "\n")
            summarize(report, args.work_dir / "summary.md")
            print(json.dumps({key: row[key] for key in ("algorithm", "mode", "cutoff", "median_seconds_per_input", "quxlang_over_cpp")}), flush=True)
    if args.verify_only:
        (args.work_dir / "verification.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
