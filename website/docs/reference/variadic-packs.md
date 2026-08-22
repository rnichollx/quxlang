# Variadic Packs

A positional variadic parameter captures zero or more call arguments as a
compile-time pack. Pack elements retain their individual values, reference
categories, and types for selection during function instantiation.

## Parameter grammar

A named positional pack begins with `%...`:

```quxlang
::count_tail FUNCTION(%first I32, %...rest I32): I32
{
  RETURN PACK_SIZE(rest) AS I32;
}
```

Only one positional pack is allowed in a parameter list. No ordinary positional
parameter may follow it. Named `@...` packs are not supported.

Use `%...IGNORED Type` when a function accepts trailing arguments without
referencing the pack:

```quxlang
::accept_prefix FUNCTION(%first I32, %...IGNORED I32): I32
{
  RETURN first;
}
```

An ignored pack has no name and cannot be passed to a pack operation.

## Call syntax

Positional arguments are supplied in a `% [...]` group:

```quxlang
ASSERT(count_tail(% [1]) == 0);
ASSERT(count_tail(% [1, 2, 3]) == 2);
```

Fixed positional parameters consume their prefix of the group; the pack
captures the remaining arguments. An empty call can bind a zero-element pack
when no fixed positional parameter is required.

Overload resolution compares fixed prefixes, pack constraints, and
non-variadic candidates. Selection is based on ordinary ranking rather than
declaration order.

## Homogeneous and deduced packs

A concrete pack type requires every element to be compatible with that type:

```quxlang
::sum_pack FUNCTION(%...values I32): I32
{
  // Expanded below.
}
```

Plain `AUTO` permits each element to retain a different type:

```quxlang
::first FUNCTION(%...values AUTO): PACK_ARG_TYPE(values, 0)
{
  RETURN PACK_ARG(values, 0);
}

ASSERT(first(% [4 AS I32, 5 AS I64]) == 4);
```

`AUTO(tag)` instead uses one deduction binding for every element:

```quxlang
::same_type_count FUNCTION(%...values AUTO(t)): I32
{
  RETURN PACK_SIZE(values) AS I32;
}
```

All captured arguments must satisfy the same `t`. Reusing that tag on other
parameters adds those parameters to the same deduction constraint.

## `PACK_SIZE`

`PACK_SIZE(pack)` produces the compile-time element count. It can participate
in `ENABLE_IF`, `STATIC_IF`, `STATIC_WHILE`, array sizes, and other constant
contexts. The name must identify a visible positional pack in the current
function instantiation.

## `PACK_ARG`

`PACK_ARG(pack, index)` selects an element value:

```quxlang
::second FUNCTION(%...values I32): I32
{
  RETURN PACK_ARG(values, 1);
}
```

The index is evaluated as a compile-time unsigned integer and must be less than
`PACK_SIZE(pack)`. Selection preserves the element's binding and reference
behavior. The pack name cannot be used directly as an ordinary expression.

## `PACK_ARG_TYPE`

`PACK_ARG_TYPE(pack, index)` is a type expression naming one element's type:

```quxlang
::copy_first FUNCTION(%...values AUTO): PACK_ARG_TYPE(values, 0)
{
  VAR result PACK_ARG_TYPE(values, 0) := PACK_ARG(values, 0);
  RETURN result;
}
```

It requires an instantiated function context, a known pack, a compile-time
index, and an in-range element. `DECLTYPE(pack)` is invalid because a pack is
not one ordinary expression with one type.

## Static expansion

Pack iteration uses compile-time expansion because indexing is compile-time:

```quxlang
::sum_pack FUNCTION(%...values I32): I32
{
  VAR result I32 := 0;
  STATIC_VAR index U64 := 0;

  STATIC_WHILE(index < PACK_SIZE(values))
  {
    result := result + PACK_ARG(values, index);
    STATIC_EVAL index++;
  }

  RETURN result;
}
```

The zero-element case generates no body. Each generation iteration substitutes
one concrete element into the emitted code.

## Constraints and failures

Pack size and types can constrain overloads:

```quxlang
::select FUNCTION(%...values I32)
  ENABLE_IF(PACK_SIZE(values) == 1 AS I32): I32
{
  RETURN 1;
}
```

Unknown pack names, direct pack use, non-constant indices, and out-of-range
`PACK_ARG` or `PACK_ARG_TYPE` selections are compilation errors. A return type
with an invalid selection fails during instantiation even when the body would
not otherwise read that element.

See [Templates](templates-and-value-parameters.md),
[Overload Resolution](overload-resolution.md), and
[Compile-Time Evaluation](compile-time-evaluation.md).
