#!/usr/bin/env python3
"""Build, validate, and time portable serial Benchmarks Game implementations."""

import argparse
from contextlib import nullcontext
import hashlib
import json
import os
import platform
from pathlib import Path
import random
import shutil
import statistics
import subprocess
import threading
import time


ROOT = Path(__file__).resolve().parent
REPOSITORY = ROOT.parents[2]
LANGUAGES = ("quxlang", "c", "cpp")


def digest(path):
    """Hash one artifact without loading large benchmark outputs into memory."""
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1048576), b""):
            result.update(block)
    return result.hexdigest()


def build(options, workloads):
    """Assemble the source bundle and compile all selected implementations."""
    bundle = options.work_dir / "bundle"
    modules = bundle / "modules"
    standard = REPOSITORY / "quxlang/tests/testdata/testbundle/modules"
    for name in ("runtime", "std", "syscall", "posix"):
        for staged in (modules / name).rglob("*.qxs"):
            if not (standard / name / staged.relative_to(modules / name)).exists():
                staged.unlink()
        shutil.copytree(standard / name, modules / name, dirs_exist_ok=True)
    shutil.copytree(ROOT / "modules/game", modules / "game", dirs_exist_ok=True)
    config = (ROOT / "qxcbuild.yml").read_text()
    host_os = {"Darwin": "macos", "Linux": "linux"}[platform.system()]
    host_cpu = {"arm64": "ARM64", "aarch64": "ARM64", "x86_64": "x64"}[platform.machine()]
    config = config.replace("platform: macos", "platform: " + host_os)
    config = config.replace("cpu: ARM64", "cpu: " + host_cpu)
    (bundle / "qxcbuild.yml").write_text(config)
    command = [str(options.qxc), str(bundle), str(options.work_dir / "quxlang"), "native"]
    with (options.work_dir / "qxc-build.log").open("w") as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
    commands = [command]
    build_names = list(workloads)
    if not options.verify_only and any(item.get("stdin") for item in workloads.values()) and "fasta" not in build_names:
        build_names.append("fasta")
    for language, compiler, suffix, standard_flag in (
        ("c", options.cc, ".c", "-std=c17"),
        ("cpp", options.cxx, ".cpp", "-std=c++20"),
    ):
        destination = options.work_dir / language
        destination.mkdir(exist_ok=True)
        for name in build_names:
            command = [compiler, standard_flag, "-O3", "-DNDEBUG", "-ffp-contract=off",
                       str(ROOT / language / (name + suffix)), "-lm", "-o", str(destination / name)]
            subprocess.run(command, check=True)
            commands.append(command)
    return commands


def invocation(options, language, name, n, stdin=False):
    """Select the executable and its native command-line parameter syntax."""
    if language == "quxlang":
        return [str(options.work_dir / "quxlang/output" / name)] + ([] if stdin else ["--n=" + str(n)])
    return [str(options.work_dir / language / name)] + ([] if stdin else [str(n)])


def verify(options, workloads):
    """Compare all three implementations with the published small reference output."""
    for name, workload in workloads.items():
        for language in LANGUAGES:
            command = invocation(options, language, name, workload["verify_n"], workload.get("stdin", False))
            input_context = (ROOT / "fixtures/fasta-1000.txt").open("rb") if workload.get("stdin") else nullcontext(None)
            with input_context as input_file:
                output = subprocess.check_output(command, stdin=input_file, timeout=options.timeout, env=options.benchmark_environment)
            actual_path = options.work_dir / (name + "-" + language + "-verification.out")
            actual_path.write_bytes(output)
            expected = (ROOT / "fixtures" / workload["expected_file"]).read_bytes() if "expected_file" in workload else workload["expected"].encode()
            if output != expected:
                raise RuntimeError(f"{name}/{language}: reference mismatch; actual output saved to {actual_path}")
        print(f"Verified {name}: Quxlang, C, and C++ match the published output.", flush=True)


def measure(options, workloads, report):
    """Time serial processes in balanced shuffled order and verify every output."""
    results = report["results"]
    randomizer = random.Random(20261003)
    for name, workload in workloads.items():
        n = workload["quick_n" if options.quick else "performance_n"]
        input_path = None
        if workload.get("stdin"):
            input_path = options.work_dir / ("fasta-input-" + str(n) + ".txt")
            with input_path.open("wb") as input_file:
                subprocess.run(invocation(options, "c", "fasta", n), stdout=input_file, check=True, timeout=options.timeout, env=options.benchmark_environment)
        samples = {language: [] for language in LANGUAGES}
        output_hash = None
        order = list(LANGUAGES)
        randomizer.shuffle(order)
        run_order = []
        # The first cycle warms filesystem and executable caches without recording timings.
        for repetition in range(options.repetitions + 1):
            sequence = order[repetition % 3:] + order[:repetition % 3]
            for language in sequence:
                path = options.work_dir / (name + "-" + language + ".out")
                command = invocation(options, language, name, n, workload.get("stdin", False))
                input_context = input_path.open("rb") if input_path else nullcontext(None)
                with path.open("wb") as output, input_context as input_file:
                    start = time.perf_counter_ns()
                    process = subprocess.Popen(command, stdin=input_file, stdout=output, env=options.benchmark_environment)
                    watchdog = threading.Timer(options.timeout, process.kill)
                    watchdog.start()
                    status = process.wait()
                    elapsed = (time.perf_counter_ns() - start) / 1e9
                    watchdog.cancel()
                    watchdog.join()
                    if status != 0:
                        raise subprocess.CalledProcessError(status, command)
                actual_hash = digest(path)
                if output_hash is None:
                    output_hash = actual_hash
                if actual_hash != output_hash:
                    raise RuntimeError(f"{name}/{language}: performance output differs")
                if repetition:
                    samples[language].append(elapsed)
                    run_order.append(language)
        medians = {language: statistics.median(values) for language, values in samples.items()}
        results[name] = {"n": n, "seconds": samples, "median_seconds": medians,
                         "output_sha256": output_hash, "run_order": run_order,
                         "input_sha256": digest(input_path) if input_path else None}
        (options.work_dir / "results.json").write_text(json.dumps(report, indent=2) + "\n")
        print(f"{name} n={n}: " + ", ".join(f"{lang} {medians[lang]:.6f}s" for lang in LANGUAGES), flush=True)
    return results


def main():
    """Parse the reproducible build and measurement configuration."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--qxc", type=Path, default=REPOSITORY / "misc/build/buildspaces/system-clang/quxlang/Release/qxc")
    parser.add_argument("--work-dir", type=Path, default=REPOSITORY / "tmp/benchmarksgame")
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--cxx", default="clang++")
    parser.add_argument("--only", nargs="+")
    parser.add_argument("--quick", action="store_true", help="Measure smaller inputs during development.")
    parser.add_argument("--verify-only", action="store_true")
    parser.add_argument("--repetitions", type=int, default=5)
    parser.add_argument("--timeout", type=int, default=600)
    options = parser.parse_args()
    if options.repetitions < 1:
        parser.error("--repetitions must be positive")
    options.work_dir = options.work_dir.resolve()
    options.qxc = options.qxc.resolve()
    options.work_dir.mkdir(parents=True, exist_ok=True)
    workloads = json.loads((ROOT / "workloads.json").read_text())
    if options.only:
        workloads = {name: workloads[name] for name in options.only}
    options.benchmark_environment = os.environ.copy()
    removed_allocator_environment = {name: options.benchmark_environment.pop(name)
                                     for name in tuple(options.benchmark_environment)
                                     if name.startswith(("Malloc", "_Malloc"))}
    commands = build(options, workloads)
    verify(options, workloads)
    report = {"removed_allocator_environment": removed_allocator_environment, "platform": platform.platform(), "machine": platform.machine(),
              "qxc": str(options.qxc), "qxc_sha256": digest(options.qxc),
              "c_version": subprocess.check_output([options.cc, "--version"]).decode(),
              "cpp_version": subprocess.check_output([options.cxx, "--version"]).decode(),
              "build_commands": commands, "quick": options.quick,
              "repetitions": options.repetitions, "results": {}}
    report["binary_sha256"] = {
        name: {language: digest(Path(invocation(options, language, name, 0)[0])) for language in LANGUAGES}
        for name in workloads
    }
    report["source_sha256"] = {
        str(path.relative_to(REPOSITORY)): digest(path)
        for source_root in (ROOT, REPOSITORY / "quxlang/tests/testdata/testbundle/modules")
        for path in sorted(source_root.rglob("*"))
        if path.is_file() and path.suffix in (".qxs", ".c", ".h", ".cpp", ".hpp", ".py", ".json", ".yml")
    }
    (options.work_dir / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    if not options.verify_only:
        measure(options, workloads, report)
    (options.work_dir / "results.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
