# Defaults, Overloads, and Enablement

## Default arguments

A parameter default follows its type:

```quxlang
::add FUNCTION(@ARG:value I32, @amount:delta I32 DEFAULT(1)): I32
{
  RETURN value + delta;
}

ASSERT(add(4) == 5);
ASSERT(add(@ARG 4, @amount 3) == 7);
```

Omitted arguments use the declaration's default expression.

## Overload sets

Declarations with the same name form an overload set:

```quxlang
::select FUNCTION(@ARG:value I32): I32
{
  RETURN 1;
}

::select FUNCTION(@ARG:value I64): I32
{
  RETURN 2;
}
```

Argument names, positional shape, conversions, template bindings, and receiver
qualification participate in candidate selection.

## `ENABLE_IF`

`ENABLE_IF` keeps the declaration present but controls whether a particular
instantiation is a viable candidate:

```quxlang
::width_class FUNCTION(@ARG:value AUTO(t))
  ENABLE_IF(BITS(t) < 32 AS I32): I32
{
  RETURN 1;
}

::width_class FUNCTION(@ARG:value AUTO(t))
  ENABLE_IF(BITS(t) >= 32 AS I32): I32
{
  RETURN 2;
}
```

## `INCLUDE_IF` versus `ENABLE_IF`

`INCLUDE_IF(condition)` controls whether a declaration enters its enclosing
scope at all. `ENABLE_IF(condition)` applies to a function candidate during
resolution. Use `INCLUDE_IF` for target or configuration availability and
`ENABLE_IF` for call-dependent constraints.

See [Templates](templates-and-value-parameters.md) and
[Availability and targets](availability-and-targets.md).

