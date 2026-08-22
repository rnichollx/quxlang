# Overview of Call Arguments

Quxlang calls use named arguments for ordinary APIs and explicit positional
groups when order is an intentional part of the interface.

## Named arguments

A named argument begins with its parameter's public name:

```quxlang
::ceil_div FUNCTION(@numerator:n I32, @denominator:d I32): I32
{
  RETURN (n + d - 1) / d;
}

VAR result I32 := ceil_div(@numerator 9, @denominator 2);
```

The declaration can give the body a shorter local name after `:`. Calls still
use `@numerator` and `@denominator`. Named arguments can be reordered without
changing which parameters they initialize:

```quxlang
VAR same_result I32 := ceil_div(@denominator 2, @numerator 9);
```

## One bare argument

A function whose single parameter is named `@ARG` can be called with one bare
expression:

```quxlang
::twice FUNCTION(@ARG I32): I32
{
  RETURN ARG * 2;
}

ASSERT(twice(6) == 12);
```

For multiple arguments, write explicit names or positional groups.

## Positional groups

Positional parameters begin with `%` in the declaration and are passed inside
`% [...]`:

```quxlang
::sum_pair FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

ASSERT(sum_pair(% [4, 5]) == 9);
```

Named arguments and positional groups can be interleaved. Their expressions
are evaluated in the order written:

```quxlang
combine(
  % [first_expression()],
  @mode selected_mode,
  % [second_expression(), third_expression()]
);
```

Constructor calls use `:(...)` for the same mixed argument grammar and `:[...]`
for a positional-only sequence.

## Reference

See the [Call Arguments Reference](../../reference/call-arguments.md) for
mapping errors, defaults, constructor forms, and overload interaction.
