# Overview of Floating-Point Ordering

Quxlang's built-in comparison operators give floating-point values a stable
total order. This is useful when floats are keys, sorted values, or fields in a
larger value:

```quxlang
VAR values [3]F32 :[0.0, -2.0, 5.0];

IF (values[0] < values[2])
{
  // The comparison participates in the same ordering model as other values.
}
```

The order distinguishes negative zero from positive zero and treats canonical
NaN values as equal to each other. Generic comparison and serialization code
can therefore use the same three-way ordering as other types.

## IEEE predicates for numerical comparisons

Some algorithms require IEEE behavior instead. In particular, IEEE comparison
treats both signed zeros as equal and treats every comparison with NaN as
unordered:

```quxlang
VAR zero F32 := 0.0;
VAR negative_zero F32 := zero * (zero - 1.0);
VAR nan F32 := zero / zero;

ASSERT(zero != negative_zero);
ASSERT(IEEE_EQUALS(zero, negative_zero));

ASSERT(nan == nan);
ASSERT(IEEE_NOTEQUALS(nan, nan));
```

The built-in operators provide value identity and ordering for containers and
generated structural operations. `IEEE_EQUALS`, `IEEE_NOTEQUALS`, `IEEE_LESS`,
and `IEEE_GREATER` provide IEEE predicate semantics for numerical algorithms.
Both forms use primitive floating-point comparisons and require no allocation.

The [Floating-Point Ordering Reference](../reference/floating-point-ordering.md)
specifies the complete special-value behavior and intrinsic requirements.
