# Overview of Overload Resolution

Overloads let several functions share a name while accepting different
arguments:

```quxlang
::select FUNCTION(@ARG:value I32): I32 { RETURN 1; }
::select FUNCTION(@ARG:value I64): I32 { RETURN 2; }

ASSERT(select(4 AS I32) == 1);
ASSERT(select(4 AS I64) == 2);
```

Quxlang first maps named and positional arguments, removes candidates that
cannot accept them, and selects the uniquely best type and reference fit.

## Concrete and generic candidates

A concrete exact match normally outranks a broader generic match:

```quxlang
::describe FUNCTION(@ARG:value I32): I32 { RETURN 1; }
::describe FUNCTION(@ARG:value AUTO(t)): I32 { RETURN 2; }
```

Reference qualifiers also matter. Temporary-reference overloads can receive
consumable values, while constant references are read-only fallbacks.

## Constrained overloads

`ENABLE_IF` keeps a candidate only when its instantiated condition is true:

```quxlang
::width_class FUNCTION(@ARG:value AUTO(t))
  ENABLE_IF(BITS(t) < 32 AS I32): I32
{
  RETURN 1;
}
```

Use `ENABLE_IF` for call-dependent constraints. Use `INCLUDE_IF` when a
declaration should not exist on a target at all.

## Reference

If no candidate works, or two candidates are equally best, the call is a
compilation error. For the exact value, reference, literal, template, receiver,
and ambiguity ordering, see the
[Overload Resolution Reference](../../reference/overload-resolution.md).
