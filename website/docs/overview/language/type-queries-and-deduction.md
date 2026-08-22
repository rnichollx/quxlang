# Overview of Type Queries

Type queries let compile-time code inspect an expression's type, compare types,
and ask about physical layout or integer properties. Deduction patterns let a
generic function bind a type from its arguments.

## Inspect a type

```quxlang
::type_queries STATIC_TEST
{
  VAR value I32 := 12;

  ASSERT(SAME_TYPES(DECLTYPE(value), I32));
  ASSERT(SAME_TYPES(TYPEOF(value), MUT& I32));
  ASSERT(SIZEOF(I32) == 4);
  ASSERT(BITS(I32) == 32);
  ASSERT(IS_SIGNED(I32));
}
```

`DECLTYPE` reports the declared value type. `TYPEOF` preserves the expression's
reference qualification. `SIZEOF`, `ALIGNOF`, and `BITS` report layout and
integer facts where the target provides them.

An empty Quxlang structure has size zero and alignment one on a physical-layout
target:

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

Guard layout queries when compiling for a layoutless target.

## Deduce an argument type

```quxlang
::identity FUNCTION(@value AUTO(t)): t
{
  RETURN value;
}
```

`AUTO(t)` binds the non-reference value type. `TT(t)` retains the presented
type, including a reference. `DECAY(t)` is useful for forwarding: it preserves
lvalue references but turns an expiring `TEMP&` into an owned value type.

Reusing a deduction name requires every occurrence to agree on the same bound
type.

## Reference

See the [Type Queries and Deduction Reference](../../reference/type-queries-and-deduction.md)
for every query, layoutless behavior, exact deduction transformations, and
overload-ranking relationships.
