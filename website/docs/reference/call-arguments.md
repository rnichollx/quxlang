# Call Arguments

Calls bind named arguments by API name and positional arguments by position.
Except for the single bare `@ARG` form, every argument explicitly begins with
`@name` or belongs to a `% [...]` positional group.

## Named arguments

```quxlang
VAR result I32 := ceil_div(@numerator 9, @denominator 2);
```

A named argument has the form `@name expression`. Named arguments need not be
written in parameter-declaration order:

```quxlang
VAR result I32 := ceil_div(@denominator 2, @numerator 9);
```

Both calls bind the same parameters. The name is the parameter's public API
name, not necessarily its body-local name:

```quxlang
::ceil_div FUNCTION(@numerator:n I32, @denominator:d I32): I32
{
  RETURN (n + d - 1) / d;
}
```

Each named argument must identify a parameter of the selected candidate and a
parameter cannot be supplied twice. Argument names are compile-time call
metadata and do not add a runtime calling-convention cost.

## The single bare `@ARG` form

A call containing exactly one unprefixed expression binds that expression to
the conventional named parameter `@ARG`:

```quxlang
::twice FUNCTION(@ARG I32): I32
{
  RETURN ARG * 2;
}

ASSERT(twice(7) == 14);
```

If a call has more than one argument, or the one parameter is not named
`@ARG`, use explicit argument syntax. A bare argument cannot be followed by
another argument.

## Positional groups

Positional expressions appear inside `% [...]`:

```quxlang
::sum FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

ASSERT(sum(% [4, 5]) == 9);
```

The expressions bind the next positional parameters in their order in the
signature. The brackets may be empty for an empty positional group. Named
arguments are not permitted inside a positional sequence, and a trailing comma
without another positional expression is rejected.

Several positional groups may appear in one call; their elements form one
logical positional sequence:

```quxlang
combine(% [first], @mode selected_mode, % [second, third]);
```

This is especially useful when a positional variadic pack surrounds or follows
named API arguments.

## Mixed calls and evaluation order

Named arguments and positional groups may be interleaved. Argument expressions
are evaluated in written source order, even though named binding is independent
of declaration order:

```quxlang
VAR combined I32 := combine(
  % [first_expression()],
  @named second_expression(),
  % [third_expression(), fourth_expression()]
);
```

The four calls occur in that order. After evaluation, the values are mapped to
the selected candidate's named and positional formal parameters.

## Empty calls and defaults

`function()` supplies no arguments. It is valid when the selected candidate has
no required parameters. A parameter omitted from the written arguments is
filled only when it declares `DEFAULT(expression)`; otherwise that candidate is
not viable.

Named arguments can omit an earlier defaulted parameter while supplying a later
named parameter. Positional arguments cannot skip an earlier required
positional slot. See [Default Arguments](default-arguments.md).

## Constructor argument forms

Direct construction and placement prefix the same argument grammar with `:`:

```quxlang
VAR named point :(@x 3, @y 4);
VAR positional point :[3, 4];
```

`:(...)` accepts the mixed named and `% [...]` call grammar. `:[...]` is a
positional-only shorthand and rejects named arguments. The same distinction is
used by constructor delegation, `PLACE AT`, and `NEW`.

## Overload interaction

Argument mapping happens for each overload candidate. Unknown names, duplicate
names, missing required parameters, too many positional values, or an
incompatible pack shape remove or reject that candidate before type ranking.
The mapped argument values then participate in [Overload Resolution](overload-resolution.md).

See [Functions](functions-and-parameters.md) for parameter declarations and
[Variadic Packs](variadic-packs.md) for consuming positional packs.
