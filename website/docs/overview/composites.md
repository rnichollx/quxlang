# Overview of Composites

A composite is a small anonymous record. It is useful when values belong
together for one operation but do not justify a named `STRUCT`.

```quxlang
VAR result AUTO := :{
  .value = 42 AS I32;
  .was_cached = TRUE;
};

IF (result.was_cached)
{
  use_cached_value(@value result.value);
}
```

Each field keeps its own type. A composite groups related values without a
runtime dictionary or a requirement that every field have the same type.

## Named option forwarding

Composites integrate with named arguments. `@KWARGS ...` captures unmatched
arguments, and `APPLY` passes composite fields to another callable:

```quxlang
::clamp FUNCTION(
  @value I32,
  @minimum I32 DEFAULT(0),
  @maximum I32 DEFAULT(100)
): I32
{
  IF (value < minimum) { RETURN minimum; }
  IF (value > maximum) { RETURN maximum; }
  RETURN value;
}

::clamp_nonnegative FUNCTION(@value I32, @KWARGS ...): I32
{
  IF (value < 0) { value := 0; }
  RETURN APPLY COMPOSITE_JOIN(
    :{ .value = value; },
    COMPOSITE_FORWARD(KWARGS)
  ) TO clamp;
}

ASSERT(clamp_nonnegative(@value 150, @maximum 80) == 80);
```

This pattern lets a wrapper consume some options and forward the rest while
preserving their names and types.

## Value and reference composites

A composite field can own a value or refer to an existing object:

```quxlang
VAR amount I32 := 12;
VAR record AUTO := :{
  .borrowed = amount;
  .owned = amount AS I32;
};

record.borrowed := 20;
ASSERT(amount == 20);
ASSERT(record.owned == 12);
```

`COMPOSITE_TIE` and `COMPOSITE_FORWARD` create records whose fields refer to
existing objects. References avoid value copies and require the source objects
to remain alive. Value fields provide independent storage and perform the
corresponding copy or move.

Composites also support positional members, field selection, splitting,
joining, compile-time reflection, and call argument expansion. The
[Composites Reference](../reference/composites.md) specifies those operations,
field order, type identity, and lifetime rules.
