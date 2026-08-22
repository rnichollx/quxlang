# Variable-Length Integer Serialization

Quxlang provides two compiler-defined unsigned variable-length encodings:
`UINTANY` and unsigned LEB128. Each family has a serializer and deserializer
that operates through language-level iterator operations.

## Built-in signatures

Conceptually, the four built-ins have these deduced interfaces:

```text
SERIALIZE_UINTANY(@VALUE CONST& U, @OUTPUT_ITERATOR O): O
DESERIALIZE_UINTANY(@VALUE MUT& U, @INPUT_ITERATOR I): I
SERIALIZE_LEB128(@VALUE CONST& U, @OUTPUT_ITERATOR O): O
DESERIALIZE_LEB128(@VALUE MUT& U, @INPUT_ITERATOR I): I
```

`U` must be `BYTE` or an unsigned integer type. Signed integers, floating-point
types, enums, and unrelated classes are rejected. The iterator type is deduced
independently for each call.

## `UINTANY`

`UINTANY` is Quxlang's offset continuation encoding. Values below 128 use one
byte. For a longer value, the serializer writes the low seven bits with bit 7
set, then continues with `(value / 128) - 1`.

```quxlang
VAR output [3]BYTE;
VAR value U64 := 70000;

SERIALIZE_UINTANY(
  @VALUE value,
  @OUTPUT_ITERATOR output[& 0]
);

ASSERT(output[0] == 240);
ASSERT(output[1] == 161);
ASSERT(output[2] == 3);
```

On decoding, each continuation level contributes the encoded seven-bit payload
and its UINTANY offset. Consequently `UINTANY` is not byte-for-byte compatible
with unsigned LEB128 even though both use bit 7 as the continuation flag.

`UINTANY` is used by Quxlang serialization contracts such as the length prefix
of a `STRINGLIKE` value.

## Unsigned LEB128

Unsigned LEB128 writes seven payload bits per byte, low group first. Bit 7 is
set when another group follows. Continuation uses `value / 128` without the
UINTANY offset.

```quxlang
VAR output [3]BYTE;
VAR value U64 := 70000;

SERIALIZE_LEB128(@VALUE value, @OUTPUT_ITERATOR output[& 0]);

ASSERT(output[0] == 240);
ASSERT(output[1] == 162);
ASSERT(output[2] == 4);
```

The two encodings agree for values below 128 and generally differ for longer
values.

## Deserialization

The destination is a mutable object of the selected unsigned type:

```quxlang
VAR encoded [2]BYTE;
encoded[0] := 200;
encoded[1] := 1;

VAR decoded U16 := 0;
VAR end AUTO := DESERIALIZE_LEB128(
  @VALUE decoded,
  @INPUT_ITERATOR encoded[& 0]
);

ASSERT(decoded == 200);
ASSERT((end - encoded[& 0]) == 2);
```

The decoder reads bytes until it encounters one whose bit 7 is clear, then
stores the accumulated result into `@VALUE`. The call takes no end iterator and
does not know the size of the underlying range. The caller must provide enough
readable bytes and a terminating byte. The input must also represent a value
appropriate for the destination width.

## Iterator protocol

The iterator is passed by value and returned after the last byte. For each byte,
the built-ins use the language-level increment and dereference operations in
the shape `(iterator++)->`.

An output iterator therefore needs:

- a usable value copy for the by-value parameter;
- postfix increment through `OPERATOR++`;
- dereference through `OPERATOR->` to a writable `BYTE` reference;
- byte assignment at that reference.

An input iterator needs the corresponding increment operation and a readable
`BYTE` reference from `OPERATOR->`.

An array address satisfies this protocol directly. A class can retain richer
iterator state:

```quxlang
VAR begin output_iterator := output_iterator(@buffer bytes[& 0]);
VAR end output_iterator := SERIALIZE_UINTANY(
  @VALUE value,
  @OUTPUT_ITERATOR begin
);

ASSERT(begin.index == 0);
ASSERT(end.index > begin.index);
```

Because the iterator argument is a value, advancing the local parameter does
not mutate the caller's iterator object unless that iterator's own semantics
share state. Capture the returned iterator to continue at the encoded end.

## Round trips and byte counts

The serializer emits the canonical shortest form produced by its quotient
loop. Its result points immediately after the encoded value. The deserializer's
result likewise points after the terminating byte, allowing adjacent values to
be chained:

```quxlang
VAR next AUTO := SERIALIZE_UINTANY(
  @VALUE first,
  @OUTPUT_ITERATOR buffer[& 0]
);
next := SERIALIZE_UINTANY(@VALUE second, @OUTPUT_ITERATOR next);
```

No implicit framing is added. If a protocol needs a byte count, range boundary,
or signed-number mapping, that contract belongs to the surrounding format.

See [Serialization](serialization.md), [Stringlike Types](stringlike-types.md),
[User-Defined Operators](user-defined-operators.md), and
[Call Arguments](call-arguments.md).
