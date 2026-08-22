# `STATIC` Compile-Time Constants

`STATIC` declares a read-only value whose initializer is evaluated during
compilation. At global scope, the value must also have a supported static
representation so the compiler can emit it into the generated program.

```quxlang
::point STRUCT
{
  .x VAR I32;
  .y VAR I32;
}

::origin_point STATIC point := point();
```

Integers, floating-point values, `BOOL`, `BYTE`, type indexes, enums, flagsets,
pointers, atomic wrappers around eligible values, interfaces, eligible attached
values and inline fusion types, arrays of eligible elements, and trivially
destructible structs of eligible fields can use Quxlang's direct static-object
contract. A plain struct uses that contract automatically when its shape
qualifies.

## `ANTESTATAL`

`ANTESTATAL` explicitly requires the direct static-object contract:

```quxlang
::offset STRUCT ANTESTATAL
{
  .dx VAR I32;
  .dy VAR I32;
}

::default_offset STATIC offset := offset();
```

The assertion is checked. An `ANTESTATAL` struct must be trivially destructible
and every field must itself support the contract. For example, adding a
nontrivial destructor makes the declaration invalid rather than silently
selecting a different representation.

The modifier is usually unnecessary for a plain structural value; use it when
the direct contract is part of the type's intended interface.

## Static references carry dependencies

Direct static values can retain references to other static objects and
procedures. Those referenced declarations become dependencies of the emitted
value:

```quxlang
::answer STATIC I32 := 42;
::answer_pointer STATIC CONST->I32 := answer<-;

::increment FUNCTION(@ARG:value I32): I32
{
  RETURN value + 1;
}

::increment_pointer STATIC
  CONST -> PROCEDURE(@ARG I32: I32) := increment<-;
```

This also applies when a pointer or procedure pointer is stored inside an
eligible inline union, inline variant, array, or struct.

## `SERIALOID`

A serialoid static value uses a serialization and deserialization contract
instead of direct static representation. `STRUCT SERIALOID` requests that
contract and asks Quxlang to generate field-wise operations when all fields
support them:

```quxlang
::saved_pair STRUCT SERIALOID
{
  .first VAR BYTE;
  .second VAR BOOL;
}

::saved_default STATIC saved_pair := saved_pair();
```

A struct with explicit `.SERIALIZE` and `.DESERIALIZE` members also qualifies
without the `SERIALOID` modifier. When reconstructing the emitted value,
Quxlang prefers a `.CONSTRUCTOR` taking `@DESERIALIZE_INPUT_ITERATOR`; otherwise it
default-constructs the object and calls `.DESERIALIZE`.

See [Serialization](serialization.md) and [Stringlike Types](stringlike-types.md) for
the member signatures and generated-operation rules.

## `NONSTATIC`

`NONSTATIC` states that objects of the struct type must not be declared
`STATIC`:

```quxlang
::runtime_resource STRUCT NONSTATIC
{
  .handle VAR UINTPTR;
}

// Rejected: the type explicitly opts out of static representation.
::invalid_resource STATIC runtime_resource;
```

The compiler diagnoses the invalid `STATIC` declaration when it is used.

## Local static forms

A function-local `STATIC` is also read-only and evaluated during compilation.
`STATIC_VAR` is a separate form: it supplies mutable state during static
expansion and must be observed from runtime code through `SNAPSHOT`.

See [Variables](variables.md) and
[Compile-Time Evaluation](compile-time-evaluation.md) for those local forms.
