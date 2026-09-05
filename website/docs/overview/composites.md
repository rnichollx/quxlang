# Overview of Composites

A composite is an anonymous struct with named fields. Use composites to return
several values, collect options, and build or forward named function arguments.

## Create a record

```quxlang
VAR record AUTO := :{
  .amount = 12 AS I32;
  .bounds = :{ .minimum = 0 AS I32; .maximum = 100 AS I32; };
};

ASSERT(record.amount == 12);
record.bounds.maximum := 80;
ASSERT(record.bounds.maximum == 80);
```

Fields use `.name = expression`, separated by semicolons. The final semicolon
is optional, and `:{}` creates an empty composite. Keyword argument names such
as `.THIS` and `.OTHER` are also valid field names.

The initializer's type determines the field's type. `12 AS I32` stores an `I32`;
an unconverted literal such as `12` retains its exact numeric literal type and
occupies zero bytes. Use a concrete numeric type when the field needs to hold
changing numbers.

## Store values or references

An expression naming a mutable local is a reference. A field initialized from
that expression therefore refers to the local:

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

`COMPOSITE_TIE` produces a record of references to the source fields:

```quxlang
VAR record AUTO := :{ .amount = 12 AS I32; };
VAR tied AUTO := COMPOSITE_TIE(record);
tied.amount := 25;
ASSERT(record.amount == 25);
```

`COMPOSITE_FORWARD` also produces references, using `TEMP&` for owned fields
accessed through a mutable source. This lets a receiving function move those
values. Existing reference fields keep their reference types. Both operations
are shallow, and the referenced objects must remain alive while used.

## Call a function with `APPLY`

`APPLY arguments TO target` passes each top-level field as a named argument:

```quxlang
::clamp FUNCTION(@value I32, @minimum I32 DEFAULT(0), @maximum I32 DEFAULT(100)): I32
{
  IF (value < minimum) { RETURN minimum; }
  IF (value > maximum) { RETURN maximum; }
  RETURN value;
}

VAR options AUTO := :{ .value = 150; .maximum = 80; };
ASSERT((APPLY options TO clamp) == 80);
```

The omitted `minimum` uses its default. `APPLY` evaluates the argument composite
first, then the target. Field initializer expressions run once in their written
order. Parenthesize an `APPLY` expression when combining its result with another
operator, as in the assertion above.

## Forward keyword arguments

`@KWARGS ...` captures otherwise unmatched named arguments as a composite.
This supports forwarding patterns similar to Python's `**kwargs`:

```quxlang
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

Here `value` binds the explicit parameter, and `maximum` becomes a field of
`KWARGS`. The declaration must put `@KWARGS ...` last. An alias such as
`@KWARGS:options ...` names the local composite `options` instead.

## Select, split, and join fields

Select fields for one consumer and pass the rest to another:

```quxlang
VAR settings AUTO := :{
  .value = 150 AS I32;
  .maximum = 80 AS I32;
  .scale = 2 AS I32;
};

VAR groups AUTO := COMPOSITE_SPLIT(COMPOSITE_TIE(settings), "scale");
ASSERT((APPLY groups.remainder TO clamp) == 80);
groups.selected.scale := 3;
ASSERT(settings.scale == 3);

VAR rejoined AUTO := COMPOSITE_JOIN(groups.selected, groups.remainder);
ASSERT(COMPOSITE_FIELD_COUNT(DECLTYPE(rejoined)) == 3);
```

`COMPOSITE_SELECT(source, "name", ...)` keeps only the named fields;
`COMPOSITE_EXCLUDE` keeps all other fields. `COMPOSITE_SPLIT` returns both groups
as `.selected` and `.remainder`. `COMPOSITE_JOIN(left, right)` combines disjoint
field sets and rejects duplicate names.

These operations construct ordinary records using copies, moves, or reference
bindings. Tying the source, as above, lets the resulting records share its fields.

## Inspect fields at compile time

Reflection supports optional arguments and static iteration over heterogeneous
records:

```quxlang
::sum_fields FUNCTION(@KWARGS ...): I32
{
  VAR total I32 := 0;
  STATIC_VAR index SZ := 0;
  STATIC_WHILE (index < COMPOSITE_FIELD_COUNT(DECLTYPE(KWARGS)))
  {
    total := total + COMPOSITE_FIELD_GET(KWARGS, index);
    STATIC_EVAL index++;
  }
  RETURN total;
}

ASSERT(sum_fields(@red 2, @green 3, @blue 5) == 10);
```

`COMPOSITE_CONTAINS` tests whether a name exists, `COMPOSITE_FIELD_NAME` gets a
name by index, and `COMPOSITE_FIELD_TYPE` gets a declared field type. Indices
follow lexicographic field-name order, starting at zero; they do not follow
initializer order.

## Reference

See the [Composites Reference](../reference/composites.md) for type identity,
reference qualifiers, reflection signatures, and call restrictions. Related
pages cover [Call Arguments](call-arguments.md), [Variadic Packs](variadic-packs.md),
and [Move Semantics](move-semantics.md).
