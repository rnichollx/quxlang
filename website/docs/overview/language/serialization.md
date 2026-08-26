# Overview of Serialization

Serialization turns a value into bytes, and deserialization reconstructs a
value from bytes. Quxlang expresses both operations with reserved member
functions and iterator arguments, so user-defined types can participate in the
same protocol as built-in types.

## Writing and reading one value

```quxlang
::byte_value STRUCT
{
  .value VAR BYTE;

  .SERIALIZE FUNCTION(@OUTPUT_ITERATOR:output MUT& AUTO(it))
  {
    (output++)-> := .value;
  }

  .DESERIALIZE FUNCTION(@INPUT_ITERATOR:input CONST=>>BYTE)
  {
    .value := input->;
  }
}
```

`.SERIALIZE` writes through `@OUTPUT_ITERATOR`. `.DESERIALIZE` reads through
`@INPUT_ITERATOR`. Postfix `++` advances an iterator, while `->` accesses the
current byte.

## Constructing from serialized input

A deserializing constructor builds a value directly from an input iterator:

```quxlang
.CONSTRUCTOR FUNCTION(@DESERIALIZE_INPUT_ITERATOR:input CONST=>>BYTE)
{
  .value := input->;
}
```

This is useful when an object cannot or should not be default-constructed before
its fields are read.

## Generating field-wise operations

Use `SERIALOID` when a structure should serialize its fields in declaration
order:

```quxlang
::record STRUCT SERIALOID
{
  .first VAR BYTE;
  .second VAR BOOL;
}
```

Every stored field must itself be serializable. For a representation that needs
versioning, validation, or a different field order, write explicit members
instead.

Serialization uses the exact static type. Polymorphic structs do not receive
automatic serialization from inheritance; see [Inheritance](inheritance.md).

Built-in integers serialize least-significant byte first. Arrays serialize in
index order. Enums and flagsets also validate their declared representations
when they are deserialized.

Variable-width integer formats are introduced under
[Integer Serialization](integer-serialization.md). Compile-time strings built
through this protocol are introduced under [Stringlike Types](stringlike-types.md).

## Reference

See the [Serialization Reference](../../reference/serialization.md) for exact
representations, generated-structure behavior, and invalid operations.
