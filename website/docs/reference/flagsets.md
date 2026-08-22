# Flagsets

A flagset is a nominal value type whose named entries are non-overlapping bit
masks. It represents sets of independently testable flags while preserving a
distinct Quxlang type.

## Declaration grammar

```text
::name FLAGSET [BITS(width)] [entries] [; | associated_body]
```

```quxlang
::access FLAGSET BITS(8) [read, write, execute = 4];
```

Entries are separated by commas. Each ordinary entry is either a name or
`name = constant_expression`. Names must be unique.

The optional `BITS` expression is evaluated at compile time and must be between
1 and 64. Without it, the width is inferred from all canonical and reserved
masks. Storage occupies `(BITS + 7) / 8` bytes.

## Explicit masks

An explicit entry fixes its canonical mask:

```quxlang
::permissions FLAGSET BITS(8) [read = 1, write = 2, execute = 4];
```

The mask must be nonzero, fit the declared width, and share no set bit with any
other canonical entry or reserved mask. Explicit masks may contain more than
one bit, but those bits become one named canonical flag and cannot be reused by
another entry.

## Implicit masks

An entry without `=` receives the lowest unoccupied single bit, considering
both earlier explicit masks and reserved bits:

```quxlang
::features FLAGSET BITS(8) [first, second, RESERVED = 4, third];
```

Here `first`, `second`, and `third` receive masks 1, 2, and 8. Allocation fails
if no unused bit remains within the declared width.

## Reserved masks

`RESERVED = expression` marks bits that cannot be assigned to canonical names:

```quxlang
::wire_flags FLAGSET BITS(16)
  [urgent = 1, RESERVED = 14, compressed = 16];
```

Reserved masks participate in width inference and overlap checking. They do not
create selectable `flagset::name` values.

## Values and membership

A named flag is selected through the flagset type:

```quxlang
VAR flags access := access::read;
```

`flags.name` tests whether every bit in that named mask is present and returns
`BOOL`:

```quxlang
ASSERT(flags.read);
ASSERT((flags.write) == FALSE);
```

The selector is defined only for canonical names declared by that flagset.

## Bitwise operations

Flagset bitwise operators preserve the nominal flagset type. They combine,
intersect, negate, and otherwise transform masks using the bitwise family:

```quxlang
flags #||= access::write;
VAR readable access := flags #&& access::read;
VAR without_write access := flags #&& access::write #!!;
```

Results are constrained to the flagset's representation width. See
[Bitwise Operators](bitwise-operators.md) for the complete operator
spellings.

## Construction and conversion

Default construction produces the zero mask. Copy and move construction
preserve the bit representation. Explicit construction or conversion can expose
or accept a compatible unsigned integer representation when raw protocol bits
are required; implicit mixing with unrelated integer or flagset types is not
provided merely because widths match.

Unknown bit patterns can exist after raw conversion. Canonical membership tests
still inspect their masks; reserved bits remain reserved by the declaration
contract even though the storage can represent them.

## Associated declarations and serialization

A flagset can use an associated declaration body instead of a trailing
semicolon:

```quxlang
::access FLAGSET BITS(8) [read, write]
{
  ::read_write STATIC access := read #|| write;
}
```

Associated declarations are selected with `access::name`. Generated
serialization and deserialization use the declared or inferred fixed width and
the flagset's nominal representation. See [Serialization](serialization.md).
