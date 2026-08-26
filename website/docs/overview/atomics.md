# Overview of Atomics

`ATOMIC#T` stores a small value that multiple threads can access without a data
race. Each operation names its memory order as a template argument, making the
synchronization choice visible at the call site.

## Publish and observe a value

```quxlang
VAR state ATOMIC#I32;
state.STORE#ATOMIC_RELEASE(7);
VAR observed I32 := state.LOAD#ATOMIC_ACQUIRE();
```

A release store and acquire load are a common publication pair. Relaxed
operations are useful when atomicity is required but the access does not carry
other memory between threads.

## Update a counter

```quxlang
VAR previous I32 := state.FETCH_ADD#ATOMIC_ACQREL(1);
state.SUB#ATOMIC_RELEASE(1);
```

`FETCH_ADD` returns the value from before the update. `ADD` performs the same
kind of update without returning the prior value. Integer and `BYTE` atomics
also provide subtraction and bitwise read-modify-write operations.

## Compare and exchange

```quxlang
VAR expected I32 := 4;
VAR exchanged BOOL := state.COMPARE_EXCHANGE#(
  @SUCCESS ATOMIC_ACQREL,
  @FAILURE ATOMIC_ACQUIRE
)(% [expected, 9]);
```

On success, the atomic becomes `9`. On failure, `expected` is replaced with the
value that was actually observed. The success and failure orders can differ,
but the failure order cannot publish memory.

Atomics supply low-level operations, not an ownership protocol. Prefer a
higher-level synchronization type when it already describes the relationship
among threads.

## Reference

See the [Atomics Reference](../reference/atomics.md) for supported payload
types, every operation, valid memory orders, and compare-exchange constraints.
