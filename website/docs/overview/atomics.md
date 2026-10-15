# Overview of Atomics

`ATOMIC#T` stores a small value that multiple threads can access atomically.
Named member functions express every access and update, and each call names
its memory order as a template argument.

## Value publication and observation

```quxlang
VAR state ATOMIC#I32;
state.STORE#ATOMIC_RELEASE(7);
VAR observed I32 := state.LOAD#ATOMIC_ACQUIRE();
```

A release store and an acquire load that observes the stored value form a
publication pair. Relaxed operations provide atomic access when the operation
does not need to publish or acquire other memory.

Assignment, swap, arithmetic, and comparison operators are unavailable on
atomic values. A named member performs an update, and `LOAD` obtains an ordinary
payload for subsequent computations or comparisons.

## Atomic counter and bit updates

```quxlang
VAR previous I32 := state.FETCH_ADD#ATOMIC_ACQREL(1);
state.SUBTRACT#ATOMIC_RELEASE(1);
state.MULTIPLY#ATOMIC_RELAXED(2);
state.BIT_OR#ATOMIC_RELAXED(16);
```

`FETCH_ADD` returns the value immediately before its update. `ADD` performs the
same update and returns `VOID`. Integer and `BYTE` atomics also provide
subtraction, multiplication, division, remainder, bitwise logic, shifts, and
rotations. Every operation has a `FETCH_*` form that returns its prior value.

The compiler uses native atomic operations where available and compare-exchange
retry loops for the remaining operations. A retry recomputes the replacement
from the current value, preserving concurrent updates. Contention can increase
the number of retries.

## Exchange and compare-exchange

```quxlang
VAR previous I32 := state.EXCHANGE#ATOMIC_ACQREL(9);
VAR expected I32 := 9;
VAR exchanged BOOL := state.COMPARE_EXCHANGE#(
  @SUCCESS ATOMIC_ACQREL,
  @FAILURE ATOMIC_ACQUIRE
)(@EXPECTED expected, @DESIRED 12);
```

`EXCHANGE` replaces the payload and returns its previous value.
`COMPARE_EXCHANGE` stores `12` when the payload equals `expected`. On failure,
`expected` receives the observed payload. The success and failure orders can
differ, but the failure order cannot publish memory.

Atomics supply low-level operations. The synchronization types in `std` provide
ownership protocols for coordinating threads.

## Reference

The [Atomics Reference](../reference/atomics.md) specifies supported payload
types, operations, memory orders, and compare-exchange constraints.
