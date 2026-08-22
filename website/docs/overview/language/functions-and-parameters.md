# Overview of Functions

A function declaration names its call parameters, optional return type, and
body:

```quxlang
::clamp FUNCTION(@value I32, @minimum I32, @maximum I32): I32
{
  IF (value < minimum)
  {
    RETURN minimum;
  }
  IF (value > maximum)
  {
    RETURN maximum;
  }
  RETURN value;
}

::offset FUNCTION(@ARG:value I32, @amount:delta I32): I32
{
  RETURN value + delta;
}
```

## Parameter forms

- `@name Type` declares a named parameter.
- `@api_name:local_name Type` separates the call-site name from the body name.
- `%name Type` declares a positional parameter.
- `%IGNORED Type` accepts one unused positional value without declaring a local
  name.
- `%...name Type` declares a positional variadic pack.
- `%...IGNORED Type` accepts an unused positional pack.

Named parameters are the primary form for ordinary APIs because each call
states which value serves each purpose. Positional parameters remain useful
when position is intrinsic to the API, especially variadic packs. Named and
positional parameters may appear in one signature; the corresponding call must
use explicit [argument groups](call-arguments.md).

## Return types

The return type follows the parameter list:

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

Omitting the return type declares a `VOID` function:

```quxlang
::reset FUNCTION(@target WRITE& I32)
{
  target := 0;
}
```

`AUTO` may be used as a deduced return type. Every reached return path must
produce a compatible result.

Member-function receiver qualifiers are covered on
[Structs and members](structs-and-members.md).

## Complete technical rules

See the [Functions Reference](../../reference/functions-and-parameters.md) for the complete
language rules, constraints, and technical edge cases.
