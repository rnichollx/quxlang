# Logical Operators

Logical operators combine or negate truth values. Quxlang also provides the
suffix presence tests `??` and `?!`, which turn the state of a supported value
into an explicit `BOOL`.

## Binary operators

Each operand of a binary logical operator must be usable as `BOOL`. The result
is always `BOOL`.

| Operator | Result | Right operand is skipped when |
| --- | --- | --- |
| `a && b` | `a` and `b` | `a` is false |
| `a &! b` | not (`a` and `b`) | `a` is false |
| `a \|! b` | not (`a` or `b`) | `a` is true |
| `a \|\| b` | `a` or `b` | `a` is true |
| `a ^^ b` | exactly one operand is true | never |
| `a ^! b` | both operands have the same truth value | never |
| `a ^> b` | `a` implies `b` | `a` is false |
| `a ^< b` | `b` implies `a` | `a` is true |

Evaluation begins with the left operand. The short-circuit cases in the table
do not evaluate the right operand. `^^` and `^!` always evaluate both operands.

```quxlang
::observe FUNCTION(@count &I32, @result BOOL): BOOL
{
  count := count + 1;
  RETURN result;
}

::short_circuit_example FUNCTION()
{
  VAR count I32 := 0;

  ASSERT((FALSE && observe(@count count, @result TRUE)) == FALSE);
  ASSERT(count == 0);

  ASSERT((TRUE || observe(@count count, @result FALSE)) == TRUE);
  ASSERT(count == 0);
}
```

Parenthesize a compound logical expression when its grouping matters. All
binary logical operators occupy the same precedence level; see
[Operator Precedence](operator-precedence.md).

## Logical negation

`value!!` is suffix logical negation. Its operand must be usable as `BOOL` and
its result is `BOOL`.

```quxlang
ASSERT((TRUE !!) == FALSE);
ASSERT((FALSE !!) == TRUE);
```

`!!` is distinct from `#!!`, the suffix bitwise inverse operator described in
[Bitwise Operators](bitwise-operators.md).

## Presence and absence tests

`value??` is the affirmative test for a value category. `value?!` is its
negative counterpart. Both return `BOOL`; neither performs an implicit
conversion at the use site.

```quxlang
VAR item ->I32;
ASSERT(item?!);

VAR number I32 := 12;
item := number<-;
ASSERT(item??);
ASSERT(item-> == 12);
```

The built-in meaning depends on the operand type:

| Category | `value??` | `value?!` |
| --- | --- | --- |
| pointer or address | nonzero | zero or null |
| integer | nonzero | zero |
| enum | nonzero representation | zero representation |
| flagset | at least one flag bit | no flag bits |
| interface value | contains an implementation | default or empty |
| union or variant that may be valueless | holds an alternative | valueless |
| never-valueless union or variant | always true | always false |

References are not nullable and therefore do not use the pointer presence
contract. Floating-point values and `BOOL` use their own operators rather than
the integer presence rules.

Presence tests are also the explicit form used in conditions:

```quxlang
IF (item??)
{
  item-> := 20;
}
```

## Operator lookup

Suffix syntax maps to reserved operator members: `!!` to `OPERATOR!!`, `??` to
`OPERATOR??`, and `?!` to `OPERATOR?!`. Built-in categories receive compiler
implementations of the applicable members. See
[User-Defined Operators](user-defined-operators.md) for reserved operator-member
names and call resolution.

