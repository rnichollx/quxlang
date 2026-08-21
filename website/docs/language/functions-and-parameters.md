# Functions and Parameters

A function declaration names its call parameters, optional return type, and
body:

```quxlang
::scale FUNCTION(%value I32, %factor I32): I32
{
  RETURN value * factor;
}

::offset FUNCTION(@ARG:value I32, @amount:delta I32): I32
{
  RETURN value + delta;
}
```

## Parameter forms

- `%name Type` declares a positional parameter.
- `@name Type` declares a named parameter.
- `@api_name:local_name Type` separates the call-site name from the body name.
- `%IGNORED Type` accepts one unused positional value without declaring a local
  name.
- `%...name Type` declares a positional variadic pack.
- `%...IGNORED Type` accepts an unused positional pack.

Named and positional parameters may appear in one signature; the corresponding
call must use explicit [argument groups](call-arguments.md).

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

