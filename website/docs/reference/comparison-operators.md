# Comparison Operators

Quxlang provides equality, relational, and three-way comparison:

| Operator | Result |
| --- | --- |
| `==` | `BOOL`, true when equal |
| `!=` | `BOOL`, true when unequal |
| `<`, `>`, `<=`, `>=` | `BOOL`, derived ordering relation |
| `<=>` | `ORDER` |

`ORDER` has the strong-order results `ORDER::LESS`, `ORDER::EQUAL`, and
`ORDER::GREATER`.

```quxlang
VAR low I32 := 2;
VAR high I32 := 5;

ASSERT(low != high);
ASSERT(low < high);
ASSERT((low <=> high) == ORDER::LESS);
```

## Built-in comparable types

The built-in comparison surface includes:

- Numeric literals, concrete integers, `BYTE`, floating-point values, and
  `BOOL`.
- Enums and flagsets of the same nominal type.
- `TYPE_INDEX` values.
- `ADDRESS` values.
- Equality for compatible non-reference pointer types.
- Strong ordering for compatible array pointers.
- Arrays and structural values whose element or field types support generated
  equality or ordering.

Primitive comparisons use compatible operand types selected by ordinary
overload resolution. Quxlang does not apply an unrequested chain of C-style
integer promotions; convert mixed concrete numeric types explicitly.

`FALSE` orders before `TRUE`. Enum and flagset ordering follows their normalized
integer representation. Array and generated structural comparisons are
lexicographic in element or field order.

## Three-way comparison

`<=>` is the canonical ordering operation. Relational operators can be derived
from its `ORDER` result:

```quxlang
VAR order ORDER := left <=> right;
IF (order == ORDER::LESS)
{
  use_left_first();
}
```

A user-defined `.OPERATOR<=>` must return `ORDER`. When it is available,
Quxlang derives `<`, `>`, `<=`, and `>=` from it. It can also derive equality
when no more direct equality overload is selected.

```quxlang
::version STRUCT
{
  .major VAR I32;
  .minor VAR I32;

  .OPERATOR<=> FUNCTION(@OTHER CONST& version): ORDER
  {
    RETURN_UNEQUAL .major, OTHER.major;
    RETURN_UNEQUAL .minor, OTHER.minor;
    RETURN ORDER::EQUAL;
  }
}
```

## Equality dispatch

For `left == right` or `left != right`, Quxlang attempts these contracts in
order:

1. An applicable `left_type::.OPERATOR==`.
2. An applicable reflected `right_type::.OPERATOR==RHS`.
3. Ordering through `.OPERATOR<=>` or `.OPERATOR<=>RHS` when available.

`.OPERATOR==` must return `BOOL`. `!=` negates the selected equality result.
For ordering-based equality, Quxlang tests whether the `ORDER` result is equal
to `ORDER::EQUAL`.

Relational operators use `.OPERATOR<=>` or reflected `.OPERATOR<=>RHS` and test
the returned ordering. Operator overload resolution uses the same parameter
qualification, conversion, priority, and `ENABLE_IF` rules as an ordinary
function call.

## Floating-point ordering

Ordinary floating-point comparison operators use Quxlang's strong total
ordering, including a defined order for signed zero and canonical NaN values.
They are intentionally different from IEEE predicate operations such as
unordered comparison. See [Floating-Point Ordering](floating-point-ordering.md)
for the exact ordering and explicit predicate family.

## Pointer comparison

Compatible instance pointers and array pointers support equality. Array
Pointers also support `<=>` and the derived relational operators. Ordering a
pointer does not make an otherwise invalid pointer dereference valid;
allocation and lifetime constraints still apply.

## Lexicographic return support

`RETURN_UNEQUAL left, right;` evaluates `left <=> right` and returns the
non-equal `ORDER`, otherwise continuing. It is documented with
[`RETURN` Statements](return-statements.md).
