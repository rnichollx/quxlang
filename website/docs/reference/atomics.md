# Atomics

`ATOMIC#T` provides atomic storage for integer, `BYTE`, `BOOL`, and pointer
payloads. Reference payloads and floating-point payloads are not supported.
Each access names its memory order through a member template argument.

Atomic values expose access and mutation through named member functions.
Assignment, swap, arithmetic, bitwise, increment, decrement, and comparison
operators are unavailable on atomic values. `LOAD` obtains a payload for
ordinary value operations. Declaration initialization from a payload remains
available; initialization does not publish the object to other threads.

## Load, store, and exchange

```quxlang
VAR state ATOMIC#I32;
state.STORE#ATOMIC_RELEASE(7);
VAR observed I32 := state.LOAD#ATOMIC_ACQUIRE();
VAR previous I32 := state.EXCHANGE#ATOMIC_ACQREL(9);
```

`LOAD#mode()` returns the stored payload. `STORE#mode(value)` stores a payload
and returns `VOID`. `EXCHANGE#mode(value)` replaces the payload and returns the
previous payload. These members are available for every supported payload type.

| Operation | Accepted modes |
| --- | --- |
| `LOAD` | `NONATOMIC`, `ATOMIC_RELAXED`, `ATOMIC_ACQUIRE`, `ATOMIC_SEQCST` |
| `STORE` | `NONATOMIC`, `ATOMIC_RELAXED`, `ATOMIC_RELEASE`, `ATOMIC_SEQCST` |
| `EXCHANGE` and integer read-modify-write members | `NONATOMIC`, `ATOMIC_RELAXED`, `ATOMIC_RELEASE`, `ATOMIC_ACQUIRE`, `ATOMIC_ACQREL`, `ATOMIC_SEQCST` |

`NONATOMIC` requests ordinary accesses through the atomic storage interface.
Concurrent conflicting accesses require external synchronization when an access
uses this mode.

## Integer read-modify-write operations

Integer and `BYTE` atomics provide the following members. Each member has the
form `operation#mode(operand)` and returns `VOID`. Each corresponding
`FETCH_operation#mode(operand)` returns the payload immediately before its
successful update. The operand has type `T`, except that shift and rotation
amounts have type `SZ`.

| Member | Updated payload, with prior value `x` and operand `y` |
| --- | --- |
| `ADD` | `x + y` |
| `SUBTRACT` | `x - y` |
| `MULTIPLY` | `x * y` |
| `DIVIDE` | Integer quotient of `x / y` |
| `MODULO` | Integer remainder of `x / y` |
| `BIT_AND` | Bitwise AND of `x` and `y` |
| `BIT_OR` | Bitwise OR of `x` and `y` |
| `BIT_XOR` | Bitwise exclusive OR of `x` and `y` |
| `BIT_NAND` | Bitwise complement of AND |
| `BIT_NOR` | Bitwise complement of OR |
| `BIT_NXOR` | Bitwise complement of exclusive OR |
| `BIT_IMPLIES` | Bitwise OR of the complement of `x` and `y` |
| `BIT_IMPLIED` | Bitwise OR of `x` and the complement of `y` |
| `BIT_SHIFT_UP` | Logical shift of `x` by `y` bits toward higher bit positions |
| `BIT_SHIFT_DOWN` | Logical shift of `x` by `y` bits toward lower bit positions |
| `BIT_ROTATE_UP` | Rotation of `x` by `y` bits toward higher bit positions |
| `BIT_ROTATE_DOWN` | Rotation of `x` by `y` bits toward lower bit positions |

```quxlang
VAR state ATOMIC#I32 := 12;
VAR previous I32 := state.FETCH_MULTIPLY#ATOMIC_ACQREL(3);
state.DIVIDE#ATOMIC_RELEASE(2);
state.BIT_OR#ATOMIC_RELAXED(1);
```

Addition, subtraction, and multiplication wrap at the payload bit width.
Division truncates toward zero, and a signed remainder has the dividend's sign.
Division or remainder by zero, and division or remainder of a signed minimum
value by negative one, produce undefined behavior. Bitwise complements are limited to the payload
bit width. Shifts at or beyond that width produce zero. Rotation amounts are
reduced modulo that width.

These members are unavailable for `BOOL` and pointer payloads. An atomic
read-modify-write operation uses a native LLVM atomic operation when available,
or a compare-exchange retry loop. Every retry computes a replacement from a
newly observed value. The returned fetch value belongs to the successful
update. Contention can cause repeated retries.

The LLVM backend supports these operations when the atomic storage width fits
the target's supported atomic storage limit. Payload widths that require
padded atomic storage retain their logical arithmetic and bitwise width.
The JVM backend does not currently support atomic read-modify-write lowering.

## Compare and exchange

Compare-exchange names success and failure orders independently:

```quxlang
VAR expected I32 := 4;
VAR exchanged BOOL := state.COMPARE_EXCHANGE#(
  @SUCCESS ATOMIC_ACQREL,
  @FAILURE ATOMIC_ACQUIRE
)(@EXPECTED expected, @DESIRED 9);
```

`EXPECTED` is a mutable payload reference, and `DESIRED` is a replacement
payload. Both operand names are required. On success, the operation stores the
desired value, preserves `expected`, and returns `TRUE`. On failure, the
operation returns `FALSE` and writes the observed payload to `expected`.

| Success mode | Accepted failure modes |
| --- | --- |
| `NONATOMIC` | `NONATOMIC` |
| `ATOMIC_RELAXED` | `ATOMIC_RELAXED` |
| `ATOMIC_RELEASE` | `ATOMIC_RELAXED` |
| `ATOMIC_ACQUIRE` | `ATOMIC_RELAXED`, `ATOMIC_ACQUIRE` |
| `ATOMIC_ACQREL` | `ATOMIC_RELAXED`, `ATOMIC_ACQUIRE` |
| `ATOMIC_SEQCST` | `ATOMIC_RELAXED`, `ATOMIC_ACQUIRE`, `ATOMIC_SEQCST` |

The synchronization types in `std` provide higher-level ownership models.
Related language features are described in
[Thread-Local Variables](thread-local-variables.md) and
[Templates and value parameters](templates-and-value-parameters.md).
