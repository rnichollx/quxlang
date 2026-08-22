# `RETURN` Statements

`RETURN` ends the current function invocation. A value-returning function
supplies an expression; a `VOID` function may return without one.

```text
RETURN expression;
RETURN;
```

## Returning a value

The declared return type follows the function parameter list:

```quxlang
::absolute FUNCTION(@ARG:value I32): I32
{
  IF (value < 0)
  {
    RETURN 0 - value;
  }
  RETURN value;
}
```

A return expression initializes the function's result object. Constructor and
argument-adaptation rules therefore apply: the expression must be usable to
construct the declared return type. This permits ordinary value conversion and
user-defined conversion constructors where their normal rules allow them; it
does not bypass explicit conversion or reference-qualification requirements.

Returning a reference preserves the declared reference category:

```quxlang
::counter STRUCT
{
  .value VAR I32;

  .get FUNCTION(): MUT& I32
  {
    RETURN .value;
  }
}
```

The referenced object must outlive the returned reference. A reference to a
function-local object cannot be used after that object is destroyed. Use
`FORWARD(reference)` when a forwarding function must preserve an incoming
`TEMP&` or other deduced reference category:

```quxlang
::forward_result FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

## `VOID` returns and fallthrough

Omitting a return type declares a `VOID` function. `RETURN;` exits it early:

```quxlang
::store_if_positive FUNCTION(@value I32, @destination WRITE& I32)
{
  IF (value <= 0)
  {
    RETURN;
  }
  destination := value;
}
```

Reaching the end of a `VOID` function also returns. A function with a concrete
non-`VOID` return type must initialize its result on every reachable exit; use
an explicit `RETURN expression;` on each such path.

## Deduced return types

`AUTO` and other template type patterns may appear in the return position:

```quxlang
::fibonacci FUNCTION(@value AUTO): AUTO
{
  IF (value < 2)
  {
    RETURN value;
  }
  RETURN fibonacci(@value value - 1) + fibonacci(@value value - 2);
}
```

The compiler matches the declared return pattern against a reached return
expression and publishes the resulting concrete return type for that function
instantiation. Every other reachable value return must be compatible with the
same result type. If a deduced-return function has no value return, its return
type is `VOID`.

`DECLTYPE(expression)` can preserve a reference category explicitly:

```quxlang
::identity FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

See [Type Queries](type-queries-and-deduction.md) for the differences among
`AUTO`, `DECLTYPE`, `TYPEOF`, `DECAY`, and `TT`.

## `RETURN_UNEQUAL`

`RETURN_UNEQUAL left, right;` is a specialized lexicographic-comparison
statement. It evaluates `left <=> right`; if the result is not
`ORDER::EQUAL`, it returns that `ORDER`. If the values compare equal, execution
continues with the next statement.

```quxlang
::compare_pair FUNCTION(
  @left_first I32,
  @right_first I32,
  @left_second I32,
  @right_second I32
): ORDER
{
  RETURN_UNEQUAL left_first, right_first;
  RETURN_UNEQUAL left_second, right_second;
  RETURN ORDER::EQUAL;
}
```

Both operands are evaluated before the three-way comparison. Their types must
support `<=>`, and the enclosing function must be able to return the resulting
`ORDER`. `RETURN_UNEQUAL` is intended for comparator implementations; it is not
a general conditional-return syntax.

## Control-flow effects

No statement after a taken `RETURN` executes. Returning transfers control to
the caller through the ordinary function result contract. Local objects and
temporaries remain governed by Quxlang's normal lifetime and destruction
rules; `RETURN` does not extend the lifetime of an object referred to by a
returned non-owning reference.

