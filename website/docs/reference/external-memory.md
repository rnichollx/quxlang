# External Memory

`IBC` marks access used for Inter Binary Communication with external data.
It is an access qualifier on pointers and references:

```quxlang
VAR view IBC CONST->U32;
VAR bytes IBC MUT=>>BYTE;
```

Write `IBC` before `MUT`, `CONST`, `TEMP`, or `WRITE` and the pointer/reference
form. Adding or removing IBC qualification requires an explicit cast in either
direction. `AUTO&` deduction preserves the incoming IBC qualification.
Copying a value through an IBC reference produces an ordinary value.

## Access propagation

Fields of `IBC_STRUCT` expose IBC-qualified access. Their declared field types
remain unchanged. IBC qualification also follows field projection, array
indexing, dereference, and address-taking through an IBC view.

When a pointer is stored in an IBC-accessed field, loading the pointer preserves
the pointer's own declared pointee qualification. IBC qualification of the field
storage does not automatically qualify the loaded pointer's target.

## Aliasing and lifetime

Ordinary Quxlang storage is accessed through its active type and valid lifetime.
There is no general byte/character or signed/unsigned aliasing exception.
`IBC` is for external data. It does not authorize constructing overlapping
ordinary objects or accessing an object after destruction. Explicit conversion
back to ordinary access requires the ordinary type and lifetime contract to hold.

## Runtime address operations

These built-ins require native addressable memory and runtime execution. They
fail if executed during constant evaluation and are unavailable on the JVM:

| Operation | Contract |
| --- | --- |
| `IBC_GETADDR(@PTR pointer)` | Export a pointer as `ADDRESS`. |
| `IBC_PUN#PointerType(@ADDR address)` | Interpret an address as the complete requested pointer type. |
| `IBC_LOAD#T(@ADDR address)` | Load an external value of type `T`. |
| `IBC_WRITE#T(@ADDR address, @VALUE value)` | Store a value of type `T` into external memory. |

`IBC_PUN` takes the complete pointer type, including any `IBC` and access
qualifiers. It rejects a reference type or a bare pointee type. The address
operand of `IBC_PUN`, `IBC_LOAD`, and `IBC_WRITE` must be `ADDRESS`.

```quxlang
::packet IBC_STRUCT
{
  .sequence VAR U32;
}

::read_packet FUNCTION(@address ADDRESS): U32
{
  VAR view IBC CONST->packet := IBC_PUN#(IBC CONST->packet)(@ADDR address);
  RETURN view->.sequence;
}

::write_sequence FUNCTION(@address ADDRESS, @sequence U32)
{
  IBC_WRITE#U32(@ADDR address, @VALUE sequence);
}
```

The caller supplies valid external storage of the required size, alignment,
representation, and access permissions. Null pointer/address conversion is
preserved by `IBC_GETADDR` and `IBC_PUN`; this does not permit a null load or
store. `NULL` also constructs a null `ADDRESS` directly.

For ordinary object storage, use the lifetime operations in
[Object Storage and Lifetime](typed-storage-and-lifetime.md).
