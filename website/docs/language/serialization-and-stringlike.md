# Serialization and Stringlike Types

Serialization is expressed through reserved member functions and iterator
arguments.

## Serialization members

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

`.SERIALIZE` writes to `@OUTPUT_ITERATOR`; `.DESERIALIZE` reads from
`@INPUT_ITERATOR`. An iterator may be returned when the operation consumes and
advances a nontrivial iterator value.

Primitive integers, floating-point types, enums, flagsets, arrays, and eligible
structs expose compatible serialization operations.

## Deserializing construction

A constructor can consume serialized input directly:

```quxlang
.CONSTRUCTOR FUNCTION(@DESERIALIZE_INPUT_ITERATOR:input CONST=>>BYTE)
{
  .value := input->;
}
```

This construction path is used when a value must be materialized from its
serialized representation without default construction first.

## `SERIALOID`

`STRUCT SERIALOID` requests field-wise generated serialization when every field
supports the required operation:

```quxlang
::record STRUCT SERIALOID
{
  .first VAR BYTE;
  .second VAR BOOL;
}
```

If the fields do not provide a valid serialization path, use explicit members
or remove the modifier. The relationship among `SERIALOID`, `ANTESTATAL`,
`NONSTATIC`, and global `STATIC` declarations is documented on
[Static objects and materialization](static-objects-and-materialization.md).

## `STRINGLIKE`

A `STRUCT STRINGLIKE` can produce a compile-time `STRING_CONSTANT` through its
serialization surface. Its serialized form begins with a `SERIALIZE_UINTANY`
length followed by exactly that many bytes:

```quxlang
::fixed_text STRUCT STRINGLIKE
{
  .SERIALIZE FUNCTION(@OUTPUT_ITERATOR:output MUT& AUTO(it))
  {
    VAR length U64 := 3;
    output := SERIALIZE_UINTANY(@VALUE length, @OUTPUT_ITERATOR output);
    (output++)-> := 'f';
    (output++)-> := 'o';
    (output++)-> := 'o';
  }
}

::text STATIC STRING_CONSTANT := fixed_text();
```

Malformed lengths, missing bytes, or trailing bytes are rejected. Conversion
to `STRING_CONSTANT` is a static operation, not a runtime string allocation.

The `UINTANY` length encoding and the parallel LEB128 built-ins are documented
on [Variable-length integer serialization](integer-serialization.md).
