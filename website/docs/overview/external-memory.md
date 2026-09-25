# Overview of External Memory

Use `IBC` pointers and references when accessing external data. For example,
an external packet can be viewed through its binary layout:

```quxlang
::packet IBC_STRUCT
{
  .sequence VAR U32;
}

::read_sequence FUNCTION(@address ADDRESS): U32
{
  VAR packet_view IBC CONST->packet :=
    IBC_PUN#(IBC CONST->packet)(@ADDR address);
  RETURN packet_view->.sequence;
}
```

The caller must supply a valid address with the required layout and alignment.
`IBC_LOAD#T` and `IBC_WRITE#T` transfer values directly; `IBC_GETADDR` exports a
pointer as an address. These four operations require runtime execution.

IBC qualification propagates through field and element access. Adding or
removing it on a pointer or reference requires an explicit cast. It is intended
for external data and does not change the active-type and lifetime rules for
ordinary Quxlang objects.

See the [External Memory Reference](../reference/external-memory.md).
