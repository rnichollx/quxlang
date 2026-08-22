# Serialization

Serialization is defined by reserved member functions whose iterator arguments
identify the input or output stream. Built-in scalar types and generated
`SERIALOID` structures use the same protocol.

## Serialization members

An explicit serializer accepts an `@OUTPUT_ITERATOR` argument:

```quxlang
::byte_value STRUCT
{
  .value VAR BYTE;

  .SERIALIZE FUNCTION(@OUTPUT_ITERATOR:output MUT& AUTO(it))
  {
    (output++)-> := .value;
  }
}
```

An explicit deserializer accepts an `@INPUT_ITERATOR` argument:

```quxlang
.DESERIALIZE FUNCTION(@INPUT_ITERATOR:input CONST=>>BYTE)
{
  .value := input->;
}
```

Serialization may return the advanced output iterator. Deserialization may
likewise return the advanced input iterator. Generated aggregate operations
thread that returned iterator from one field operation to the next.

The iterator must support the operations used by the serializer. Byte-oriented
iterators conventionally expose the current byte through `iterator->` and move
to the next byte through postfix `iterator++`.

## Deserializing construction

A constructor can reconstruct an object directly from serialized input:

```quxlang
.CONSTRUCTOR FUNCTION(@DESERIALIZE_INPUT_ITERATOR:input CONST=>>BYTE)
{
  .value := input->;
}
```

This is a construction path: the object does not have to be default-constructed
and then assigned. The reserved argument name distinguishes the constructor
from ordinary constructors.

## Built-in representations

The built-in representation rules are:

- integers and `BYTE` use a fixed number of bytes, with the least-significant
  byte first; widths that are not a multiple of eight are rounded up to bytes;
- `BOOL` uses one byte;
- floating-point values are canonicalized before serialization and after
  deserialization;
- enums use the byte width of their declared storage type;
- flagsets use their declared storage width and require unused padding bits to
  be zero when deserialized;
- arrays serialize their elements in index order.

Enum deserialization rejects a representation that does not name an enumerator
unless the enum permits unknown values with `ALLOW_UNKNOWN`.

Variable-width integer formats are separate operations documented under
[Integer Serialization](integer-serialization.md).

## Generated structure serialization

`STRUCT SERIALOID` requests generated field-wise serialization:

```quxlang
::record STRUCT SERIALOID
{
  .first VAR BYTE;
  .second VAR BOOL;
}
```

Fields are processed in field-list order. Only storage-bearing fields
participate. An attached field is serialized using the storage supplied by its
carrier. Every participating field must provide the required serialization
operation; otherwise the generated operation is unavailable and the program
must provide an explicit implementation or remove `SERIALOID`.

`SERIALOID` does not itself make a type stringlike. The additional stringlike
contract is documented under [Stringlike Types](stringlike-types.md).

## Invalid operations

A serialization expression is invalid when the selected type has no matching
reserved operation, the iterator cannot perform the required read or write, or
a generated aggregate contains an unsupported field. Deserialization also
rejects representations that violate a type-specific validity rule, including
unknown closed-enum values and nonzero flagset padding bits.

See [Static Compile-Time Constants](static-compile-time-constants.md) for the
relationship among `SERIALOID`, `ANTESTATAL`, `NONSTATIC`, and global `STATIC`
declarations.
