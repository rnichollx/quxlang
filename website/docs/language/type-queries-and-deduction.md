# Type Queries and Deduction

Quxlang exposes type identity, expression type, layout, and integer facts as
compile-time expressions:

```quxlang
::type_queries STATIC_TEST
{
  VAR value I32 := 12;

  ASSERT(SAME_TYPES(DECLTYPE(value), I32));
  ASSERT(SAME_TYPES(TYPEOF(value), MUT& I32));
  ASSERT(SIZEOF(I32) == 4);
  ASSERT(BITS(I32) == 32);
  ASSERT(IS_SIGNED(I32));
  ASSERT(IS_INTEGRAL(U32));
  ASSERT(BIT 5 == 32);
  ASSERT(TYPE_INDEX_OF(I32) != TYPE_INDEX_OF(I64));

  STATIC_IF(ARCH_IS_LAYOUTLESS == FALSE)
  {
    ASSERT(ALIGNOF(I32) == 4);
  }
}
```

## Type identity

- `DECLTYPE(symbol)` returns the declared value type.
- `TYPEOF(expression)` preserves the expression form, including reference
  qualification.
- `SAME_TYPES(left, right)` compares two types.
- `TYPE_INDEX_OF(T)` produces the stable program type identity used by
  `TYPE_INDEX` values.

## Layout and integer properties

- `SIZEOF(T)` gives byte size where the type has a known size.
- `ALIGNOF(T)` gives alignment and is unavailable for layoutless types.
- `BITS(T)` gives an integer type's bit width.
- `IS_SIGNED(T)` and `IS_INTEGRAL(T)` report integer properties.
- `TYPE_IS_LAYOUTLESS(T)` reports whether the current target omits a static byte
  layout for `T`.
- `BIT n` produces the compile-time numeric literal `2` raised to bit index
  `n`; cast it when a concrete runtime integer type is required.

## Deduction

`AUTO` deduces a type from its use. A repeated named form such as `AUTO(t)`
binds the same deduced type in every place that uses `t`:

```quxlang
::identity FUNCTION(@ARG:value AUTO(t)): t
{
  RETURN value;
}
```

`DECAY(name)` is the forwarding-oriented deduction pattern. It preserves
mutable and constant lvalue references but removes `TEMP&` from an expiring
argument:

```quxlang
::materialize FUNCTION(@ARG:value AUTO& DECAY(result)): result
{
  RETURN FORWARD(value);
}
```

Use `AUTO` when the exact matched shape is required and `DECAY` when an
expiring reference should become an owned value type.

See [Templates](templates-and-value-parameters.md),
[Variadic packs](variadic-packs.md), and
[Availability and targets](availability-and-targets.md).
