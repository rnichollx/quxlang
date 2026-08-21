# Unions and Variants

Quxlang fusion types represent one active alternative. A union names its
alternatives; a variant identifies alternatives by type.

## Unions

```quxlang
::status INLINE_UNION
{
  .ok OPTION DEFAULT VOID;
  .error OPTION I32;
}

VAR success status;
VAR failure status :(@error 5);

ASSERT(success IS ok);
ASSERT(failure IS error);
```

The constructor argument name selects a union option. A `VOID` option has no
payload.

## Variants

```quxlang
::number_or_void INLINE_VARIANT [I32 DEFAULT, VOID];

VAR number number_or_void := 7 AS I32;
VAR nothing number_or_void := NULL;

ASSERT(number ISA I32);
ASSERT(nothing ISA VOID);
ASSERT((UNWRAP number INTO I32) == 7);
```

`ISA T` tests the active variant type. `UNWRAP value INTO T` returns its payload
and faults when `T` is not active.

## Inline and boxed storage

- `INLINE_UNION` and `INLINE_VARIANT` store payload data inline.
- `UNION` and `VARIANT` use boxed payload storage and can represent recursive
  shapes that an inline type cannot contain directly.

## Lifecycle modifiers

Fusion declarations support `NEVER_VALUELESS`, `VALUELESS_DEFAULT`,
`NO_DEFAULT_COPY`, `NO_DEFAULT_MOVE`, `NO_DEFAULT_ASSIGN`, and
`NO_DEFAULT_SWAP`.

`NEVER_VALUELESS` requires construction and replacement to preserve an active
alternative. `VALUELESS_DEFAULT` makes default construction produce no active
alternative. The `NO_DEFAULT_*` forms suppress the corresponding generated
operation.

Observation and exhaustive handling are covered on [`MATCH`](match.md).

