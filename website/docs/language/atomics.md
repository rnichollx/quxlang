# Atomic Objects

`ATOMIC#T` provides atomic storage for integer, `BYTE`, and `BOOL` payloads.
The memory order is a template argument on each operation, so it is visible at
the call site.

## Load and store

```quxlang
VAR state ATOMIC#I32;
state.STORE#ATOMIC_RELEASE(7);
VAR observed I32 := state.LOAD#ATOMIC_ACQUIRE();
```

The available access-mode names are `NONATOMIC`, `ATOMIC_RELAXED`,
`ATOMIC_RELEASE`, `ATOMIC_ACQUIRE`, `ATOMIC_ACQREL`, and `ATOMIC_SEQCST`.
Their valid unary uses are:

| Operation | Accepted modes |
| --- | --- |
| `LOAD` | `NONATOMIC`, `ATOMIC_RELAXED`, `ATOMIC_ACQUIRE`, `ATOMIC_SEQCST` |
| `STORE` | `NONATOMIC`, `ATOMIC_RELAXED`, `ATOMIC_RELEASE`, `ATOMIC_SEQCST` |

`NONATOMIC` requests an ordinary access through the atomic storage interface;
it does not provide inter-thread synchronization.

## Integer read-modify-write operations

Integer and `BYTE` atomics provide operations that return the previous value:

```quxlang
VAR prior I32 := state.FETCH_ADD#ATOMIC_ACQREL(1);
prior += state.FETCH_SUB#ATOMIC_ACQREL(2);
prior += state.FETCH_AND#ATOMIC_ACQREL(255);
prior += state.FETCH_OR#ATOMIC_ACQREL(16);
prior += state.FETCH_XOR#ATOMIC_ACQREL(3);
```

The corresponding `ADD`, `SUB`, `AND`, `OR`, and `XOR` members perform the same
atomic transformation without returning the previous value:

```quxlang
state.ADD#ATOMIC_RELEASE(1);
state.XOR#ATOMIC_RELEASE(3);
```

These arithmetic and bitwise members are not supplied for `ATOMIC#BOOL`.
Select one of `ATOMIC_RELAXED`, `ATOMIC_RELEASE`, `ATOMIC_ACQUIRE`,
`ATOMIC_ACQREL`, or `ATOMIC_SEQCST` for a read-modify-write operation.
`NONATOMIC` does not define a supported read-modify-write lowering.

## Compare and exchange

Compare-exchange names success and failure orders independently:

```quxlang
VAR expected I32 := 4;
VAR exchanged BOOL := state.COMPARE_EXCHANGE#(
  @SUCCESS ATOMIC_ACQREL,
  @FAILURE ATOMIC_ACQUIRE
)(% [expected, 9]);
```

The first positional operand is a mutable expected value; the second is the
desired replacement. On success, the operation stores the desired value and
returns `TRUE`. On failure, it returns `FALSE` and writes the observed value
back to the mutable expected object. A failure order cannot be `ATOMIC_RELEASE`
or `ATOMIC_ACQREL`, and it cannot require stronger synchronization than the
success order. `NONATOMIC` is valid only when both orders are `NONATOMIC`.

Atomic operations do not by themselves define a higher-level ownership
protocol. Use them to implement or support a documented synchronization
contract, and use the `std` synchronization types when their ownership model
fits the task.

See [Concurrency and per-thread storage](concurrency-and-per-thread-storage.md)
and [Templates and value parameters](templates-and-value-parameters.md).
