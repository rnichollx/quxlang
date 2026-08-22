# Overview of `RETURN` Statements

`RETURN` ends a function call and optionally supplies its result. Use
`RETURN expression;` in a value-returning function and `RETURN;` to leave a
`VOID` function early.

## Returning values

```quxlang
::minimum FUNCTION(@left I32, @right I32): I32
{
  IF (left < right)
  {
    RETURN left;
  }
  RETURN right;
}
```

The returned expression must be usable to construct the declared return type.
Normal conversion and constructor rules apply.

Omitting the return type makes a function `VOID`:

```quxlang
::write_positive FUNCTION(@value I32, @destination WRITE& I32)
{
  IF (value <= 0)
  {
    RETURN;
  }
  destination := value;
}
```

## Deduced and reference results

`AUTO` lets a function instantiation deduce its result type from a return
expression:

```quxlang
::twice FUNCTION(@value AUTO): AUTO
{
  RETURN value + value;
}
```

A reference return names existing storage rather than copying the object:

```quxlang
::box STRUCT
{
  .value VAR I32;

  .get FUNCTION(): MUT& I32
  {
    RETURN .value;
  }
}
```

The referenced storage must outlive the returned reference. A forwarding
function uses `FORWARD` with a reference-preserving return type:

```quxlang
::identity FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

## Lexicographic comparisons

`RETURN_UNEQUAL` compares two expressions with `<=>` and returns the result
only when they differ. This keeps multi-field comparators concise:

```quxlang
::compare_pair FUNCTION(@left_a I32, @right_a I32,
                        @left_b I32, @right_b I32): ORDER
{
  RETURN_UNEQUAL left_a, right_a;
  RETURN_UNEQUAL left_b, right_b;
  RETURN ORDER::EQUAL;
}
```

## Complete technical rules

See the [`RETURN` Statements Reference](../../reference/return-statements.md)
for result construction, deduced returns, reference lifetime requirements,
fallthrough, and the exact `RETURN_UNEQUAL` behavior.

