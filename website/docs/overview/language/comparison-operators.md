# Overview of Comparison Operators

Equality operators return `BOOL`; the three-way operator `<=>` returns
`ORDER::LESS`, `ORDER::EQUAL`, or `ORDER::GREATER`.

```quxlang
VAR low I32 := 2;
VAR high I32 := 5;

ASSERT(low != high);
ASSERT(low < high);
ASSERT(low <= high);
ASSERT((low <=> high) == ORDER::LESS);
```

## Three-way comparison

`<=>` is useful when code needs the relationship itself:

```quxlang
VAR relationship ORDER := left <=> right;
IF (relationship == ORDER::GREATER)
{
  use_right_first();
}
```

User-defined types can provide `.OPERATOR<=>`; Quxlang derives `<`, `>`, `<=`,
and `>=` from its `ORDER` result.

```quxlang
::coordinate STRUCT
{
  .value VAR I32;

  .OPERATOR<=> FUNCTION(@OTHER CONST& coordinate): ORDER
  {
    RETURN .value <=> OTHER.value;
  }
}
```

Define `.OPERATOR==` when equality has a more direct contract. Quxlang derives
`!=` by negating equality.

## Comparable categories

Primitive numbers, `BOOL`, enums, flagsets, type indexes, addresses, compatible
pointers, arrays, and eligible structural values provide built-in or generated
comparison. Floating-point operators use Quxlang's strong total ordering.

## Complete technical rules

See the [Comparison Operators Reference](../../reference/comparison-operators.md)
for supported categories, overload dispatch order, generated lexicographic
comparison, pointer constraints, and floating-point ordering boundaries.

