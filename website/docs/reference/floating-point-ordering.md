# Floating-Point Ordering

Quxlang provides two floating-point comparison families:

| Family | Operations | Semantics |
| --- | --- | --- |
| Value ordering | `==`, `!=`, `<`, `>`, `<=`, `>=`, `<=>` | Strong total order used by Quxlang values |
| IEEE predicates | `IEEE_EQUALS`, `IEEE_NOTEQUALS`, `IEEE_LESS`, `IEEE_GREATER` | IEEE equality and ordered relational predicates |

## Value ordering

The built-in operators compare floating-point values using Quxlang's strong
value order. For finite values and infinities, the order is:

```text
-infinity < negative finite values < -0 < +0
          < positive finite values < +infinity < NaN
```

All supported NaN encodings are canonicalized to one quiet representation by
the language's value semantics. Canonical NaN compares equal to itself and to
another canonical NaN:

```quxlang
VAR zero F32 := 0.0;
VAR one F32 := 1.0;
VAR nan F32 := zero / zero;

ASSERT(nan == nan);
ASSERT((nan != nan) == FALSE);
ASSERT((nan <=> nan) == ORDER::EQUAL);
ASSERT(nan > (one / zero));
```

Negative and positive zero are distinct values:

```quxlang
VAR positive_zero F32 := 0.0;
VAR negative_zero F32 := positive_zero * (positive_zero - 1.0);

ASSERT(negative_zero != positive_zero);
ASSERT(negative_zero < positive_zero);
ASSERT((negative_zero <=> positive_zero) == ORDER::LESS);
```

`<=>` returns `ORDER::LESS`, `ORDER::EQUAL`, or `ORDER::GREATER`. The six
Boolean operators agree with that ordering. Arrays and generated structural
comparisons use the same floating-point value order when they compare fields or
elements.

## IEEE predicate syntax

```text
IEEE_EQUALS(left, right)
IEEE_NOTEQUALS(left, right)
IEEE_LESS(left, right)
IEEE_GREATER(left, right)
```

Each intrinsic requires exactly two positional floating-point arguments of the
same type and returns `BOOL`. Named arguments and mixed floating-point types are
not accepted.

| Intrinsic | Result |
| --- | --- |
| `IEEE_EQUALS(left, right)` | `TRUE` when the operands are IEEE equal |
| `IEEE_NOTEQUALS(left, right)` | `TRUE` when the operands are unequal or unordered |
| `IEEE_LESS(left, right)` | `TRUE` when `left` is ordered before `right` |
| `IEEE_GREATER(left, right)` | `TRUE` when `left` is ordered after `right` |

## Special values under IEEE predicates

The IEEE predicates treat `-0` and `+0` as equal:

```quxlang
ASSERT(IEEE_EQUALS(negative_zero, positive_zero));
ASSERT(IEEE_NOTEQUALS(negative_zero, positive_zero) == FALSE);
ASSERT(IEEE_LESS(negative_zero, positive_zero) == FALSE);
ASSERT(IEEE_GREATER(positive_zero, negative_zero) == FALSE);
```

If either operand is NaN, equality, less-than, and greater-than are `FALSE`,
while not-equal is `TRUE`:

```quxlang
ASSERT(IEEE_EQUALS(nan, positive_zero) == FALSE);
ASSERT(IEEE_NOTEQUALS(nan, positive_zero));
ASSERT(IEEE_LESS(nan, positive_zero) == FALSE);
ASSERT(IEEE_GREATER(nan, positive_zero) == FALSE);
```

This behavior is symmetric in the operand positions. Infinities otherwise
participate in IEEE ordered comparison in the usual way.

## Comparison family selection

The built-in operators apply when a floating-point value requires reflexive
equality and strong ordering, including sorting, generated comparison, and
serialization. The IEEE predicates apply when signed-zero equivalence or NaN
unordered behavior forms part of the numerical algorithm's contract.

[Comparison Operators](comparison-operators.md) specifies operator dispatch and
[`ORDER`](comparison-operators.md#three-way-comparison).
[Primitive Types and Literals](primitive-types-and-literals.md) specifies the
supported floating-point types.
