# User-Defined Operators

A struct or interface can expose selected operations through function members
whose reserved names begin with `OPERATOR`. The operator expression is still
parsed with the language's fixed precedence and associativity; overloading only
selects the operation performed for its operands.

## Operator-member declarations

An operator member uses the same declaration rules as another function member:

```quxlang
::counter STRUCT
{
  .value VAR I32;

  .OPERATOR+ FUNCTION(@OTHER I32): counter
  {
    VAR result counter;
    result.value := .value + OTHER;
    RETURN result;
  }
}
```

The leading `.` gives the declaration an implicit `@THIS` receiver. Explicit
parameters describe the remaining operands; binary operators conventionally
use `@OTHER`. Receiver and parameter qualifiers participate in ordinary
overload resolution, so a mutating operator must request a writable receiver
and a read-only operator can accept a const receiver.

The result type must be appropriate for the expression using the operator.
The compiler does not infer an operator's meaning from its name or repair an
incompatible signature.

## Binary lookup and `RHS`

For a general binary expression `left op right`, Quxlang first tries
`left`'s `OPERATORop` with `left` as `@THIS` and `right` as `@OTHER`. If that is
not callable, it tries `right`'s `OPERATORop RHS`, reversing the operands passed
as `@THIS` and `@OTHER`.

```quxlang
::distance STRUCT
{
  .millimetres VAR I32;

  .OPERATOR+ RHS FUNCTION(@OTHER I32): distance
  {
    VAR result distance;
    result.millimetres := OTHER + .millimetres;
    RETURN result;
  }
}
```

The `RHS` spelling is part of the reserved member name, not a parameter name.
It lets a type define an operation when it appears on the right without adding
a global operator declaration. If both candidates are callable, the left-hand
member is selected first.

## Equality and ordering

Comparison declarations are restricted to `OPERATOR==` and `OPERATOR<=>`.
Declaring `OPERATOR!=`, `OPERATOR<`, `OPERATOR>`, `OPERATOR<=`, or
`OPERATOR>=` is a syntax error.

`OPERATOR==` must return `BOOL`. It supplies both `==` and the negated `!=`
operation. `OPERATOR<=>` must return `ORDER`; it supplies `<=>` and the four
relative comparisons.

```quxlang
::rank STRUCT
{
  .value VAR I32;

  .OPERATOR<=> FUNCTION(@OTHER CONST& rank): ORDER
  {
    IF (.value < OTHER.value) { RETURN ORDER::LESS; }
    IF (.value > OTHER.value) { RETURN ORDER::GREATER; }
    RETURN ORDER::EQUAL;
  }
}
```

The three results are `ORDER::LESS`, `ORDER::EQUAL`, and `ORDER::GREATER`.
Comparison lookup also supports the `RHS` form when the right operand owns the
applicable implementation.

`RETURN_UNEQUAL left, right;` implements a lexicographic comparison step. It
evaluates `left <=> right`; a non-equal result is returned immediately, while
`ORDER::EQUAL` continues to the next statement.

```quxlang
::compare_pair FUNCTION(@left_a I32, @right_a I32,
                        @left_b I32, @right_b I32): ORDER
{
  RETURN_UNEQUAL left_a, right_a;
  RETURN_UNEQUAL left_b, right_b;
  RETURN ORDER::EQUAL;
}
```

## Call, indexing, and dereference

The following reserved members provide postfix operations:

| Expression | Member selected | Arguments |
| --- | --- | --- |
| `object(...)` | `OPERATOR()` | call arguments after `@THIS` |
| `object[...]` | `OPERATOR[]` | bracket arguments after `@THIS` |
| `object[& ...]` | `OPERATOR[&]` | bracket arguments after `@THIS` |
| `object->` | `OPERATOR->` | only `@THIS` |

`OPERATOR()` participates in normal named and positional argument binding.
`OPERATOR[]` and `OPERATOR[&]` are distinct names; the address-index form does
not automatically reuse the ordinary index overload. Iterator-like types
normally return a reference from `OPERATOR->` so the loop or caller can access
the current element.

```quxlang
::box STRUCT
{
  .value VAR I32;

  .OPERATOR[] FUNCTION(@index SZ): MUT& I32
  {
    ASSERT(index == 0);
    RETURN .value;
  }
}
```

## Mutation operators

Assignment, swap, compound assignment, and increment or decrement use reserved
operator members as described on their dedicated pages:

- `OPERATOR:=` implements copy-style assignment;
- `OPERATOR<->` implements swap;
- `OPERATOR++` and `OPERATOR--` implement prefix increment and decrement;
- `OPERATOR++ RHS` and `OPERATOR-- RHS` implement postfix forms;
- compound operators such as `OPERATOR+=` receive the right operand as
  `@OTHER`.

The language also synthesizes operations for built-in and eligible aggregate
types. A user declaration competes through the same overload machinery rather
than changing the syntax of the expression. See
[Assignment Operators](assignment-operators.md),
[Increment and Decrement](increment-and-decrement.md), and
[Swap Operator](swap-operator.md).

## Logical and presence operators

Logical expressions have language-defined truth evaluation and
short-circuiting. Suffix expressions use the reserved names `OPERATOR!!`,
`OPERATOR??`, and `OPERATOR?!`; `#!!` uses `OPERATOR#!!`. Built-in types receive
the applicable implementations from the compiler. See
[Logical Operators](logical-operators.md) and
[Bitwise Operators](bitwise-operators.md).

## Fixed grammar and overload selection

An `OPERATOR` declaration must use an operator token accepted by the lexer;
`OPERATOR()`, `OPERATOR[]`, and `OPERATOR[&]` use their paired delimiter
spellings. Whitespace may appear between `OPERATOR` and its token. `RHS`, when
present, follows that token.

Operator overloads use the ordinary function candidate and parameter-ranking
rules. They cannot introduce a new token, change precedence, make an invalid
operand expression parse, or replace control-flow semantics such as logical
short-circuiting. See [Overload Resolution](overload-resolution.md) and
[Operator Precedence](operator-precedence.md).
