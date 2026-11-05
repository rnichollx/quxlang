# Direct coroutine implementation benchmarks

This suite compares C++20 and Quxlang coroutine invocation, allocation,
completion, destruction, and suspension costs through deeply nested calls.
Execution uses one calling thread. A parent drives each child to completion
before continuing with the next child.

## Workloads

| Algorithm | Default input | Work per leaf |
| --- | --- | --- |
| Quicksort | 1,024 deterministic unsigned values | An empty or single-element range returns immediately. Every partition recursively invokes two children. |
| Hashed reduction | 1,024 indices | One index is hashed and returned. Every internal node invokes two children and adds their results. |
| N-queens | An 8-by-8 board | A completed board returns one; a dead end returns zero. Every legal placement invokes a child coroutine through the full search depth. |

Each workload has three implementations:

- `ordinary` uses ordinary recursive functions with the corresponding algorithm.
- `coroutine` uses a heap-owning coroutine for each recursive invocation. Children
  complete through direct drivers, and their parents continue in the same activation.
- `suspend` uses the same coroutine tree and explicitly suspends once at the start
  of every invocation. Its direct driver resumes that invocation before proceeding.

The default leaf cutoff is one. N-queens creates coroutines through every search
level. The full call tree therefore exercises many short invocations without
substantial sequential work hidden in leaves. Quicksort includes partition work
at internal nodes; reduction provides a more uniform comparison of short calls.

## Runtime and allocation

The C++ owner in `runtime.hpp` contains a coroutine handle. Its promise stores the
result and an exception pointer. `initial_suspend` and `final_suspend` preserve the
frame for explicit driving and destruction. Child awaits drive the child inline
and return `false` from `await_suspend` after completion.

The Quxlang controller specializes each child, creates an owning
`std::coroutine_handle#descriptor`, initializes its frame, and drives `RESUME`
until completion. It returns the child's result through an immediately ready
awaiter. The root uses the same allocation and driving policy.

Frame allocation and destruction are included in timing. Both compilers use
normal Release optimization, including any allocation elision they can prove.
C++ uses its default allocator; Quxlang uses its standard allocator. These costs
are part of the implementation comparison. Neither implementation creates worker
threads, queues, shared result states, or synchronization primitives.

## Execution

The Quxlang compiler must have a Release build. From the repository root:

```sh
cbuild build -c Release -t quxlang -w misc/build
python3 quxlang/benchmarks/coroutines/run.py
```

The runner assembles a separate bundle under `tmp/coroutine-direct`. Quxlang uses
Release mode and DEFAULT steppings without overrides. C++ uses
`clang++ -std=c++20 -O3 -DNDEBUG` with its default CPU target. Native execution is
validated on macOS ARM64.

`--build-only` builds the executables. `--skip-build` checks the recorded compiler
and benchmark source identities before reusing the binaries. `--verify-only`
checks all selected configurations without measuring samples.

```sh
python3 quxlang/benchmarks/coroutines/run.py --skip-build --verify-only
python3 quxlang/benchmarks/coroutines/run.py --skip-build \
  --size 4096 --queens-size 9
```

`--algorithms` selects `quicksort`, `reduction`, or `queens`. `--cutoffs` controls
the maximum sequential leaf size for quicksort and reduction. `--queens-depths`
controls how many search levels create coroutines and defaults to the board size.
Larger leaves measure application-level tradeoffs and are outside the default
coroutine-dense comparison.

## Measurements and interpretation

Input allocation and generation, output validation, and checksum calculation are
outside timing. Each interval includes the complete root invocation and all
child frame lifetimes. A relaxed atomic input-size load precedes each interval
to prevent invariant work from being hoisted out of the measured invocation.

The runner measures complete groups of eight input seeds. Each pair of languages
uses a common calibrated group count. Three recorded samples per language each
contain at least three seconds of measured execution. Warmup is discarded and
execution order rotates. Every result is checked against independent Python
sorting, a flat reduction, or exact N-queens solution counts.

`results.json` records source, compiler, and binary hashes, build commands,
verification, calibration, raw samples, and the number of invocations in each
input's call tree. Invocation counts are computed outside timing. `summary.md`
reports nanoseconds per invocation, the Quxlang/C++ ratio, and the difference
from the same language's ordinary-function baseline per invocation. That
difference is an aggregate overhead estimate that includes allocation,
optimization, frame management, and the direct driver. It is not a measurement
of one isolated coroutine instruction.

The ordinary-function rows establish how much time the algorithm itself takes.
The coroutine and suspend rows provide the implementation comparison. Differences
between coroutine and suspend expose the additional cost of one explicit
suspension per invocation within these implementations.
