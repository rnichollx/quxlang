# Parallel quicksort benchmark

This benchmark compares two Quxlang quicksort implementations: recursive
`std::thread` creation and `GO` using `std::thread_pool_cort_runtime`. Both implementations
partition a half-open range around the middle element, launch the right
partition, recursively sort the left partition, and wait
for the right partition. Ranges at or below the configured cutoff use serial
quicksort. Neither implementation limits launch depth.

The `std::thread` implementation creates a new native thread for every launched
right partition. The `GO` implementation queues those partitions on a pool with
the hardware concurrency number of workers. The calling thread sorts the
root range in the `std::thread` case and waits while a pool worker sorts the
root range. The benchmark measures these complete execution paths, including
thread creation or queueing, synchronization, and the final join or future
completion. Input generation and sorted-output validation are outside timing.
Ordinary recursive coroutine calls also use the pool controller's child-call
protocol, which queues the left partition and awaits its completion.

Run from the repository root after building QXC in Release mode:

```sh
python3 quxlang/benchmarks/parallel_quicksort/run.py
```

`--size`, `--cutoff`, and `--repetitions` select the workload. The default
workload sorts 1,024 deterministic unsigned values with a two element
serial cutoff. The runner independently checks sorted checksums, calibrates a
common pass count, alternates measurement order, and records five samples.
Quxlang uses Release mode and DEFAULT steppings without overrides. Results
and build identities are written under `tmp/parallel-quicksort`.

## macOS ARM64 result, 2026-11-08

The local Release QXC build produced both variants. The pool used 16 workers.
Each of three samples included 128 complete sorts of 1,024 values with a two
element cutoff. Median times were:

| Quxlang implementation | Median per sort |
| --- | ---: |
| `std::thread` for right partitions | 4.371 ms |
| `GO` with 16 pool workers | 0.860 ms |

The GO median was 0.197 times the `std::thread` median, approximately 5.1
times faster in this workload. The measurement includes the cost of creating
native threads for right partitions and the cost of scheduling those partitions
on an existing pool. Input generation and output validation occur outside the
timed interval.

