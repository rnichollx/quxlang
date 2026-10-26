# Allocator benchmarks

## Bulk allocation and recursive deallocation

The `run_bulk_lifetimes.py` runner compares Quxlang and C++ with mimalloc on a
single-threaded connected-node workload. Each cycle constructs a balanced binary
tree containing exactly `floor(memory_budget / node_size)` separately allocated
nodes. No node is freed during construction. The subsequent postorder traversal
recursively deletes the complete tree. Balanced subtrees keep recursion depth
logarithmic in the node count.

The following command measures a 16 GiB live-node target with 64-byte, 1 KiB,
and 8 KiB nodes:

```sh
python3 quxlang/benchmarks/allocators/run_bulk_lifetimes.py \
  --mimalloc-source misc/build/sources/mimalloc \
  --memory-mib 16384 --sizes 64 1024 8192
```

The budget includes child pointers and payload within live node objects.
Allocator metadata, retained free allocations, and other process memory remain
outside that budget. The target is not an RSS limit. Supported node sizes are
32, 64, 256, 1024, 4096, and 8192 bytes. The runner requires a supported 64-bit
native host.

Both implementations zero-initialize each node before assigning its children
and payload identities. Each node receives a consecutive identity through
opaque writes at 4096-byte payload intervals and at the final payload word.
Recursive deletion reads the same
words and returns their checksum and the number of deleted nodes. The runner
verifies both values against an independent arithmetic reference. Small
verification cases include incomplete trees and budgets with trailing bytes.

Allocation and deallocation have separate timers. Allocation includes node
construction, linking, and payload writes. Deallocation includes traversal,
checksum accumulation, and deletion. Each executable starts with its allocator's
initial state; first-use costs are included. Calibration increases the number
of complete build-and-destroy cycles, rather than the peak live-node target.
Every recorded sample contains at least three seconds of accumulated allocation
work and three seconds of accumulated deallocation work. Implementations use
identical cycle counts, interleaved execution, and three repetitions by default.

The default output directory is `tmp/allocator-comparison-out/bulk-lifetimes`.
Its `results.json` records phase samples, median phase and total seconds per
cycle, calibration history, checksums, build commands, and source and executable
hashes. The `--build-only` option compiles both implementations without running
the workload. The `--memory-mib`, `--cycles`, `--repetitions`, and `--min-seconds`
options control the workload and measurement duration.
