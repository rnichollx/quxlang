# Overview of Integer Serialization

Quxlang provides two compiler-defined unsigned integer encodings through
ordinary iterator-shaped calls: `UINTANY` and unsigned `LEB128`.

## `UINTANY`

`SERIALIZE_UINTANY` writes an unsigned value and returns the advanced output
iterator. `DESERIALIZE_UINTANY` writes the decoded value through `@VALUE` and
returns the advanced input iterator:

```quxlang
::uintany_roundtrip STATIC_TEST
{
  VAR output [2]BYTE;
  VAR original U16 := 200;
  VAR decoded U16 := 0;

  SERIALIZE_UINTANY(
    @VALUE original,
    @OUTPUT_ITERATOR output[& 0]
  );
  DESERIALIZE_UINTANY(
    @VALUE decoded,
    @INPUT_ITERATOR output[& 0]
  );

  ASSERT(decoded == original);
}
```

`UINTANY` is Quxlang's compact unsigned-integer serialization used by contracts
such as the length prefix of a `STRINGLIKE` value. It is not byte-for-byte
identical to LEB128.

## Unsigned `LEB128`

The corresponding LEB128 forms have the same call interface:

```quxlang
VAR output [3]BYTE;
VAR value U64 := 70000;
VAR decoded U64 := 0;

SERIALIZE_LEB128(@VALUE value, @OUTPUT_ITERATOR output[& 0]);
DESERIALIZE_LEB128(@VALUE decoded, @INPUT_ITERATOR output[& 0]);

ASSERT(decoded == value);
```

The value parameter must be an unsigned integer type or `BYTE`. Deserialization
needs a mutable destination object of that type.

## Iterator return values

Both families take the iterator by value and return its advanced value. Capture
that result when the caller needs to continue serializing or when the iterator
is a nontrivial object:

```quxlang
VAR end AUTO := SERIALIZE_UINTANY(
  @VALUE value,
  @OUTPUT_ITERATOR begin
);
```

This convention lets an array pointer work as an iterator while preserving the
state of a richer iterator object passed by value.

See [Serialization](serialization.md), [Stringlike Types](stringlike-types.md), and
[Call arguments](call-arguments.md).

## Reference

See the [Variable-Length Integer Serialization Reference](../reference/integer-serialization.md) for the complete
language rules, constraints, and technical edge cases.
