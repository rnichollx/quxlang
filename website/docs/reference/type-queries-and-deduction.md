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

## Composite metadata

`COMPOSITE_CONTAINS`, `COMPOSITE_FIELD_COUNT`, `COMPOSITE_FIELD_NAME`, and
`COMPOSITE_FIELD_TYPE` inspect a composite type. `COMPOSITE_FIELD_GET` projects
a field from a value. Names and indices are selected at compile time; indices
use canonical field-name order. See [Composite Reflection](composites.md#static-reflection)
for the exact signatures and access qualifiers.

## Layout and integer properties

- `SIZEOF(T)` gives byte size where the type has a known size.
- `ALIGNOF(T)` gives alignment and is unavailable for layoutless types.
- `BITS(T)` gives an integer type's bit width.
- `IS_SIGNED(T)` and `IS_INTEGRAL(T)` report integer properties.
- `TYPE_IS_LAYOUTLESS(T)` reports whether the current target omits a static byte
  layout for `T`.
- `BIT n` produces the compile-time numeric literal `2` raised to bit index
  `n`; cast it when a concrete runtime integer type is required.

An empty Quxlang struct has size zero and alignment one on targets with a
physical layout:

```quxlang
::empty STRUCT
{
}

::empty_layout STATIC_TEST
{
  STATIC_IF(ARCH_IS_LAYOUTLESS == FALSE)
  {
    ASSERT(SIZEOF(empty) == 0);
    ASSERT(ALIGNOF(empty) == 1);
  }
}
```

Quxlang does not add a synthetic byte to make distinct empty objects occupy
different addresses. Arrays and containing structs therefore preserve the
zero-sized placement of an empty element or field unless another field imposes
size or alignment. `SIZEOF` and `ALIGNOF` are unavailable when the target is
layoutless.

## Deduction patterns

`AUTO(t)` matches the non-reference form of the type presented to it. Reusing
the name `t` requires every occurrence to bind the same type:

```quxlang
::identity FUNCTION(@ARG:value AUTO(t)): t
{
  RETURN value;
}
```

`TT(t)` matches the presented type as-is, including a reference qualifier. It
is the direct type-template pattern used when reference identity itself must
participate in matching:

```quxlang
::accept_direct_type FUNCTION(@ARG:value TT(t)): BOOL
{
  RETURN TRUE;
}
```

For overload ranking, an exact non-template reference match ranks before a
direct `TT` template match. A direct `TT` match ranks before adding a constant
reference qualification.

`DECAY(t)` is the forwarding-oriented deduction pattern. It preserves mutable
and constant lvalue references but removes `TEMP&` from an expiring argument:

```quxlang
::decay_value FUNCTION(@ARG:value AUTO& DECAY(result)): result
{
  RETURN FORWARD(value);
}
```

Use `AUTO(t)` to deduce the non-reference value type, `TT(t)` to retain the
presented reference shape, and `DECAY(t)` when an expiring reference should
become an owned value type while lvalue references remain references.

See [Templates](templates-and-value-parameters.md),
[Variadic packs](variadic-packs.md), and
[Availability and targets](availability-and-targets.md).
