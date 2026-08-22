# Overview of Variadic Packs

A positional variadic parameter begins with `%...` and must be the last
positional parameter:

```quxlang
::count_tail FUNCTION(%first I32, %...rest I32): I32
{
  RETURN PACK_SIZE(rest) AS I32;
}

::second_tail FUNCTION(%first I32, %...rest I32): I32
{
  RETURN PACK_ARG(rest, 1);
}

ASSERT(count_tail(% [1, 2, 3]) == 2);
ASSERT(second_tail(% [1, 2, 7]) == 7);
```

## Pack operations

- `PACK_SIZE(pack)` is the compile-time number of arguments.
- `PACK_ARG(pack, index)` selects one argument.
- `PACK_ARG_TYPE(pack, index)` names that argument's type.

A heterogeneous `AUTO` pack preserves each argument type:

```quxlang
::first FUNCTION(%...values AUTO): PACK_ARG_TYPE(values, 0)
{
  RETURN PACK_ARG(values, 0);
}
```

`AUTO(tag)` on the pack instead requires every element to bind the same deduced
type:

```quxlang
::same_type_count FUNCTION(%...values AUTO(t)): I32
{
  RETURN PACK_SIZE(values) AS I32;
}
```

Static expansion can iterate a pack:

```quxlang
::sum_pack FUNCTION(%...values I32): I32
{
  VAR result I32 := 0;
  STATIC_VAR index U64 := 0;
  STATIC_WHILE(index < PACK_SIZE(values))
  {
    result += PACK_ARG(values, index);
    STATIC_EVAL index++;
  }
  RETURN result;
}
```

An out-of-range `PACK_ARG` or `PACK_ARG_TYPE` is a compilation error.

## Reference

See the [Variadic Packs Reference](../../reference/variadic-packs.md) for the complete
language rules, constraints, and technical edge cases.
