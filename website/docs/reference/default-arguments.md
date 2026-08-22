# Default Arguments

A function parameter can provide an expression used when a call omits that
argument.

## Declaration syntax

`DEFAULT(expression)` follows the parameter type:

```quxlang
::add FUNCTION(
  @ARG:value I32,
  @amount:delta I32 DEFAULT(1)
): I32
{
  RETURN value + delta;
}
```

Defaults can be attached to named or positional parameters. They are not a
separate overload declaration and do not change the parameter's declared type.

## Omitting arguments

A call may omit a parameter that has a default:

```quxlang
ASSERT(add(4) == 5);
ASSERT(add(@ARG 4, @amount 3) == 7);
```

Supplying the argument uses the supplied expression; the default is not also
evaluated. Named calls can omit an earlier defaulted parameter while supplying
a later named parameter:

```quxlang
::combine FUNCTION(@x I32 DEFAULT(11), @y I32): I32
{
  RETURN x + y;
}

ASSERT(combine(@y 3) == 14);
```

A missing parameter without a default makes that candidate non-viable.

## Evaluation context

The default expression is resolved in the declaration's lexical context, not
the caller's context:

```quxlang
::default_amount VAR I32 := 7;

::use_default FUNCTION(@value I32 DEFAULT(default_amount)): I32
{
  RETURN value;
}

::example FUNCTION(): I32
{
  VAR default_amount I32 := 100;
  RETURN use_default(); // Uses the declaration-level object.
}
```

The expression is evaluated as part of the call when the argument is omitted.
Its side effects and failures therefore occur only on calls that use it.

## Overloads and templates

Argument mapping first determines whether each overload can receive the written
arguments and fill every omitted parameter. The resulting viable candidates
then participate in [Overload Resolution](overload-resolution.md).

Defaults belong to their individual declarations. Two overloads can provide
different defaults, but if omitting arguments leaves both candidates equally
good, the call is ambiguous; defaults are not a declaration-order tiebreaker.

For a template or generic function, names and types used by the default are
resolved with the selected instantiation context. The resulting expression must
initialize the parameter type for that instantiation.

## Restrictions

`DEFAULT` requires parentheses and one expression. The expression must be valid
for the parameter type whenever a selected call uses it. A default cannot make
an unknown call-site argument name valid and cannot fill an element of a
variadic pack individually.

See [Call Arguments](call-arguments.md) for named and positional mapping.
