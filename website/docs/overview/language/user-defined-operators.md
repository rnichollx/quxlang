# Overview of User-Defined Operators

Operator members use reserved `.OPERATOR...` names.

## Comparison operators

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

An `.OPERATOR<=>` definition can supply `<`, `>`, `<=`, and `>=` through
comparison synthesis. `.OPERATOR==` similarly supplies equality and inequality.

`RETURN_UNEQUAL` shortens lexicographic three-way comparison:

```quxlang
::compare_pair FUNCTION(@left_a I32, @right_a I32,
                        @left_b I32, @right_b I32): ORDER
{
  RETURN_UNEQUAL left_a, right_a;
  RETURN_UNEQUAL left_b, right_b;
  RETURN ORDER::EQUAL;
}
```

Each statement returns immediately when its operands are unequal and otherwise
continues to the next field.

## Other operator members

Types may declare the operator members supported by their role, including:

- `.OPERATOR()` for callable objects;
- `.OPERATOR->` for iterator dereference;
- `.OPERATOR+=` and `.OPERATOR-=` for iterator movement;
- `.OPERATOR:=` for assignment;
- `.OPERATOR<->` for swap.

The declaration's parameters and receiver qualifier define the valid operand
types. Operator overloading does not change precedence or invent unrelated
control flow.

## Reference

See the [User-Defined Operators Reference](../../reference/user-defined-operators.md) for the complete
language rules, constraints, and technical edge cases.
