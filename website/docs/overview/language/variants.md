# Overview of Variants

A variant holds one of several payload types. Use one when the type of the
payload is enough to identify each possible state.

## Declaring alternatives

```quxlang
::number_or_void INLINE_VARIANT [I32 DEFAULT, VOID];
```

This variant can contain an `I32` or the payload-free `VOID` state. `DEFAULT`
makes `I32` the alternative selected by the no-argument constructor.

## Constructing values

Initialize a variant from a value of one of its alternatives:

```quxlang
VAR number number_or_void := 7 AS I32;
VAR nothing number_or_void := NULL;
```

The `I32` expression selects the `I32` alternative. `NULL` selects `VOID`.

## Testing and unwrapping

Use `ISA` to test the active type:

```quxlang
IF (number ISA I32)
{
  VAR payload I32 := UNWRAP number INTO I32;
  ASSERT(payload == 7);
}
```

`UNWRAP` accesses the payload and fails if the requested type is not active.
Use it when the active type is already guaranteed. Use `MATCH` when the program
needs to branch safely over every alternative:

```quxlang
MATCH number AS payload
{
  TYPE I32 { consume(@value payload); }
  TYPE VOID { handle_empty(); }
}
```

## Inline and boxed variants

`INLINE_VARIANT` stores its payload inside the object. `VARIANT` uses boxed
storage and can express a directly recursive alternative. Both forms use the
same construction, `ISA`, `UNWRAP`, and `MATCH` syntax.

## Valueless variants

After some lifecycle operations, an ordinary variant may have no active
alternative. `value??` reports an active value; `value?!` reports the valueless
state.

`NEVER_VALUELESS` requests a type that keeps an alternative active.
`VALUELESS_DEFAULT` instead makes the no-argument constructor intentionally
produce the valueless state.

For unique-type constraints, constructor selection, unwrap failures, lifecycle
modifiers, and generated operations, see the
[Variants Reference](../../reference/variants.md).
