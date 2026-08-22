# Stringlike Types

`STRINGLIKE` marks a structure whose serialized byte sequence can be converted
at compile time to `STRING_CONSTANT`.

## Declaration

The marker follows `STRUCT`:

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
```

`STRINGLIKE` is a structure tag. It does not add storage and does not generate a
serializer. The structure must provide a usable `.SERIALIZE` operation.

## Serialized form

The byte sequence has exactly two parts:

1. a `SERIALIZE_UINTANY` byte length;
2. exactly that many content bytes.

The length counts bytes, not source characters or Unicode scalar values. An
empty value encodes a zero length and no content bytes.

## Conversion to `STRING_CONSTANT`

The conversion is available during compile-time evaluation:

```quxlang
::text STATIC STRING_CONSTANT := fixed_text();

::text_properties STATIC_TEST
{
  ASSERT(text.BEGIN()-> == 'f');
  ASSERT((text.END() - text.BEGIN()) == 3);
}
```

The compiler evaluates the serializer, decodes its length prefix, and constructs
the constant string. This is not a runtime string allocation and does not make a
runtime local `STRINGLIKE` object implicitly convertible to `STRING_CONSTANT`.

## Validation

Conversion fails when:

- the type has no usable serialization member;
- the length prefix is malformed or cannot be represented;
- fewer bytes follow than the declared length;
- bytes remain after the declared content;
- the serialized sequence does not terminate correctly; or
- the declared length is too large for the constant representation.

The exact `UINTANY` length format is specified under
[Integer Serialization](integer-serialization.md). The general reserved-member
protocol is specified under [Serialization](serialization.md).
