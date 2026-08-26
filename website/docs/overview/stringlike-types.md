# Overview of Stringlike Types

A `STRINGLIKE` structure can produce a `STRING_CONSTANT` during compile-time
evaluation. This lets a user-defined value serve as compile-time text without
creating a runtime string object.

## Define a stringlike value

The serializer writes a `UINTANY` byte length followed by exactly that many
bytes:

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

`STRINGLIKE` marks the contract; the structure still supplies its own
`.SERIALIZE` member.

## Form a compile-time string

```quxlang
::text STATIC STRING_CONSTANT := fixed_text();

::text_properties STATIC_TEST
{
  ASSERT(text.BEGIN()-> == 'f');
  ASSERT((text.END() - text.BEGIN()) == 3);
}
```

The length is a byte length. Empty strings are valid: encode a zero length and
write no content bytes.

## Avoid malformed encodings

The conversion requires exactly the promised number of bytes. Too few bytes,
trailing bytes, a malformed length, or a missing serializer makes the
compile-time conversion fail. A runtime local stringlike object is not a
runtime allocation or an implicit runtime conversion to `STRING_CONSTANT`.

See [Serialization](serialization.md) for the underlying member protocol and
[Integer Serialization](integer-serialization.md) for `UINTANY`.

## Reference

See the [Stringlike Types Reference](../reference/stringlike-types.md) for
the exact serialized form and conversion validity rules.
