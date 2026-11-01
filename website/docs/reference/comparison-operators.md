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

Generated comparison also applies to `VARIANT`, `INLINE_VARIANT`, `UNION`, and
`INLINE_UNION` when every non-`VOID` alternative supports generated structural
comparison. Structures and arrays containing those values receive comparison
under the same recursive eligibility rule. A declared comparison operator takes
precedence over generation.

Fusion equality compares active alternative identity and then the active
payload. Two valueless objects compare equal. A valueless object orders before
an active object. Different active alternatives order by their declaration
index. Equal alternative indexes order by their payloads; a `VOID` alternative
has no payload to compare. Boxed representation does not affect comparison.

Polymorphic structs are not implicitly datatypes and do not receive generated
equality or three-way comparison. They may declare an explicit comparison when
one static-type contract is meaningful; inheritance does not make comparison
virtual. See [Inheritance](inheritance.md).

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

All ordinary instance or array pointers of a given type support `==`, `!=`,
`<=>`, `<`, `<=`, `>`, and `>=`. Pointer comparisons define a total order,
including null pointers,
past-the-end pointers, and pointers whose allocations are no longer valid.
Comparing pointers does not cause undefined behavior. These rules also apply
during constant evaluation.

Within an allocation, pointers are ordered by element index. The past-the-end
pointer compares greater than every element pointer in that allocation.
Ordering between allocations is unspecified but consistent. All comparison
operators agree with the same order, including transitivity and equality.

Whether a past-the-end pointer compares equal to another pointer is unspecified
but consistent, subject to the allocation rules below. Pointers into distinct
allocations whose allocation lifetimes overlap compare unequal, except that a
past-the-end pointer may compare equal to the start of another allocation.
Whether a pointer from an allocation that is no longer valid compares equal to
a pointer from an allocation whose allocation lifetime did not overlap is
unspecified but consistent.

Comparison equality does not imply pointer identity. Equal pointers can retain
distinct allocation dependencies. Equality does not establish a dependency on
another allocation or permit access to that allocation. Access through such a
pointer requires the applicable `LAUNDER` operation to establish the required
dependency. Allocation validity, object lifetime, and dereference constraints
continue to apply independently of comparison results.

## Lexicographic return support

`RETURN_UNEQUAL left, right;` evaluates `left <=> right` and returns the
non-equal `ORDER`, otherwise continuing. It is documented with
[`RETURN` Statements](return-statements.md).
