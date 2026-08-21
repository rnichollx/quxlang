# Flagsets

A flagset is a nominal set of named bit masks:

```quxlang
::access FLAGSET BITS(8) [read, write, execute = 4]
{
  ::read_write STATIC access := read #|| write;
}

VAR flags access := access::read;
flags #||= access::write;

ASSERT(flags.read);
ASSERT(flags.write);
ASSERT(flags == access::read_write);
```

Implicit entries receive available single-bit masks. An explicit `= value`
fixes a mask. `BITS(N)` fixes the stored and serialized width.

## Membership and combination

`flags.name` tests whether the named mask is present. The bitwise family
combines, intersects, inverts, and otherwise transforms flagset values while
preserving the nominal flagset type:

```quxlang
VAR readable access := flags #&& access::read;
VAR without_write access := flags #&& access::write #!!;
```

Explicit casts convert between a flagset and a compatible unsigned integer
representation when the program needs the raw mask.

Flagsets provide `.SERIALIZE` and `.DESERIALIZE` using the width declared by
`BITS(N)`.

