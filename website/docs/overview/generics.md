# Overview of Generics

Generics describe a set of object operations while erasing the concrete object
type.

## Generic references

`GENERIC_REF` is a non-owning erased reference:

```quxlang
::mutable_counter_ref GENERIC_REF
{
  .add FUNCTION(@ARG I32) MUT: I32;
  .read FUNCTION() CONST: I32;
}

::readable_ref GENERIC_REF CONST
{
  .read FUNCTION() CONST: I32;
}
```

The receiver suffix states the access required by each operation. A
`GENERIC_REF CONST` surface itself carries only constant access.

## Owning generics

`GENERIC` owns the erased object and manages its lifetime:

```quxlang
::owning_counter GENERIC
{
  .add FUNCTION(@ARG I32) MUT: I32;
  .read FUNCTION() CONST: I32;
}
```

An object whose member functions satisfy the surface can initialize the
generic. Copying, moving, comparing, and destroying the generic apply the
corresponding supported operation to its current concrete value.

`GENERIC MOVE_ONLY` suppresses copying. `GENERIC INCOMPARABLE` suppresses the
generated cross-value comparison surface.

## Surface reuse

`IMPLEMENTS` includes another generic or generic-reference surface:

```quxlang
::implemented_counter GENERIC
{
  IMPLEMENTS mutable_counter_ref;
}
```

The included operations remain part of the declaring generic's public surface.

## Explicit receiver form

The suffix form follows the same function model as an explicit receiver:

```quxlang
.read FUNCTION(@THIS CONST& THISTYPE): I32;
```

For ordinary declarations, `.read FUNCTION() CONST: I32;` is the preferred
compact spelling. Both retain formal `THISTYPE`; a concrete object type is used
when the operation is instantiated.

Generics expose their public object contract only. Their storage and dispatch
representation is not a source-language interface.

## Reference

See the [Generics Reference](../reference/generics.md) for the complete
language rules, constraints, and technical edge cases.
