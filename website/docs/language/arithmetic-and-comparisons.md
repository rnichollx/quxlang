# Arithmetic and Comparisons

## Arithmetic

```quxlang
VAR total I32 := left + right;
VAR difference I32 := left - right;
VAR product I32 := left * right;
VAR quotient I32 := left / right;
VAR remainder I32 := left % right;
```

The implemented arithmetic family is `+`, `-`, `*`, `/`, and `%`. Integer
width, signedness, and conversion rules remain explicit; a narrowing result
requires an appropriate [conversion mode](conversions.md).

## Comparisons

```quxlang
ASSERT(left != right);
ASSERT(left <= right);
ASSERT((left <=> right) == ORDER::LESS);
```

The comparison family is `==`, `!=`, `<`, `>`, `<=`, `>=`, and `<=>`.
Three-way comparison returns `ORDER`, whose ordered values include
`ORDER::LESS`, `ORDER::EQUAL`, and `ORDER::GREATER`.

Quxlang can synthesize relational operators from a user-defined
`.OPERATOR<=>`, and inequality from `.OPERATOR==`. See
[User-defined operators](user-defined-operators.md).

Floating-point operators use Quxlang's strong value ordering. The explicit
IEEE predicate family is documented on
[Floating-point ordering](floating-point-ordering.md).

## Pointer arithmetic

Array pointers support addition, subtraction, comparison, increment, and
decrement. Instance pointers do not become multi-object pointers merely because
their target has a size; the `->T` and `=>>T` categories remain distinct.
