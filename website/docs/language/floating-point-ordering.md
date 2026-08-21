# Floating-Point Ordering

Quxlang's ordinary floating-point comparison operators provide a strong value
ordering. This keeps generic equality, sorting, structured comparison, and
serialization composable without a floating-point special case.

## Ordinary comparisons

Ordinary `==`, `!=`, `<`, `>`, `<=`, `>=`, and `<=>` use Quxlang's value
semantics:

```quxlang
VAR positive_zero F32 := 0.0;
VAR minus_one F32 := positive_zero - 1.0;
VAR negative_zero F32 := positive_zero * minus_one;

ASSERT(positive_zero != negative_zero);
ASSERT(negative_zero < positive_zero);

VAR nan F32 := positive_zero / positive_zero;
ASSERT(nan == nan);
ASSERT(nan <= nan);
ASSERT(nan >= nan);
```

Positive and negative zero are distinct ordered values. NaN compares equal to
itself in the ordinary Quxlang ordering.

## IEEE predicates

Numerical algorithms that require IEEE comparison behavior use explicit
predicate functions:

```quxlang
ASSERT(IEEE_EQUALS(positive_zero, negative_zero));
ASSERT(IEEE_NOTEQUALS(positive_zero, negative_zero) == FALSE);

ASSERT(IEEE_EQUALS(nan, nan) == FALSE);
ASSERT(IEEE_NOTEQUALS(nan, nan));
ASSERT(IEEE_LESS(nan, positive_zero) == FALSE);
ASSERT(IEEE_GREATER(nan, positive_zero) == FALSE);
```

The current predicate family is:

| Function | IEEE relation |
| --- | --- |
| `IEEE_EQUALS(left, right)` | equal |
| `IEEE_NOTEQUALS(left, right)` | not equal |
| `IEEE_LESS(left, right)` | less than |
| `IEEE_GREATER(left, right)` | greater than |

Use ordinary operators for Quxlang value ordering and the named predicates only
where IEEE unordered behavior is part of the algorithm's contract.

See [Arithmetic and comparisons](arithmetic-and-comparisons.md),
[Primitive types and literals](primitive-types-and-literals.md), and
[Value semantics](../philosophy/values.md).
