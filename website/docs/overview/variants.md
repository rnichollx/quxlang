# Overview of Variants

A variant holds one of several payload types. The payload type identifies the
active state.

## Alternative declarations

```quxlang
::number_or_void INLINE_VARIANT [I32 DEFAULT, VOID];
```

This variant can contain an `I32` or the payload-free `VOID` state. `DEFAULT`
makes `I32` the alternative selected by the no-argument constructor.

## Variant construction

Initialize a variant from a value of one of its alternatives:

```quxlang
VAR number number_or_void := 7 AS I32;
VAR nothing number_or_void := NULL;
```

The `I32` expression selects the `I32` alternative. `NULL` selects `VOID`.

## Active-type queries and payload access

The `ISA` operator tests the active type:

```quxlang
IF (number ISA I32)
{
  VAR payload I32 := UNWRAP number INTO I32;
  ASSERT(payload == 7);
}
```

`UNWRAP` accesses the payload and fails if the requested type is not active.
Direct `UNWRAP` access is appropriate when the surrounding program establishes
the active type. The `MATCH` statement branches over every alternative:

```quxlang
MATCH number AS payload
{
  TYPE I32 { consume(@value payload); }
  TYPE VOID { handle_empty(); }
}
```

[`VISIT`](visit.md) compiles the same source region once for each non-`VOID`
payload type and allows overload resolution to select type-specific behavior.

## Inline and boxed variants

`INLINE_VARIANT` stores its payload inside the object. `VARIANT` uses boxed
storage and can express a directly recursive alternative. Both forms use the
same construction, `ISA`, `UNWRAP`, and `MATCH` syntax.

## Valueless variants

After some lifecycle operations, an ordinary variant may have no active
alternative. `value??` reports an active value; `value!?` reports the valueless
state.

`NEVER_VALUELESS` requests a type that keeps an alternative active.
`VALUELESS_DEFAULT` instead makes the no-argument constructor intentionally
produce the valueless state.

## Reference

For unique-type constraints, constructor selection, unwrap failures, lifecycle
modifiers, and generated operations, see the
[Variants Reference](../reference/variants.md).
