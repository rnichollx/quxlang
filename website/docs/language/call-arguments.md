# Call Arguments

Quxlang makes positional and named arguments explicit at the call site.

## Positional groups

Positional arguments belong to `% [...]`:

```quxlang
VAR product I32 := scale(% [6, 7]);
```

`scale(6, 7)` is invalid. The grouping prevents a reader from having to recover
the positional-versus-named API from the callee.

## Named arguments

A named argument uses `@name expression`:

```quxlang
VAR shifted I32 := offset(@ARG 10, @amount 3);
```

The argument name is the parameter's API name, not necessarily its body-local
name.

## Interleaving and evaluation order

Named arguments and positional groups may be interleaved. Expressions are
evaluated in source order:

```quxlang
VAR combined I32 := combine_interleaved_arguments(
  % [first_expression()],
  @named second_expression(),
  % [third_expression(), fourth_expression()]
);
```

## The one-argument shorthand

Exactly one bare expression is shorthand for `@ARG`:

```quxlang
::square FUNCTION(@ARG:value I32): I32
{
  RETURN value * value;
}

VAR squared I32 := square(5); // square(@ARG 5)
```

The shorthand never selects a `%value` positional parameter. Calls with two or
more arguments must spell every named argument or positional group.

Compiler forms such as `SAME_TYPES(...)`, `STATIC_CHOOSE(...)`, and
`ASSERT(condition, message)` have dedicated grammars and are not ordinary
function calls. The `IEEE_EQUALS(left, right)`, `IEEE_NOTEQUALS(left, right)`,
`IEEE_LESS(left, right)`, and `IEEE_GREATER(left, right)` predicates also accept
their two bare operands as a dedicated built-in form.

Constructor argument forms are covered on [Arrays and construction](arrays-and-construction.md)
and [Constructors and destructors](constructors-and-destructors.md).
