# Addresses and Provenance

`ADDRESS` is Quxlang's raw address value. Pointer types carry additional type
and access information, so moving between the two is always explicit.

## Discovering an externally established object

`ADDRESS_LAUNDER_DISCOVER_EXISTING` obtains a pointer to an object whose
lifetime and storage were established outside the current allocation region:

```quxlang
::atomic_word_at FUNCTION(@address ADDRESS): MUT->ATOMIC#U32
{
  RETURN ADDRESS_LAUNDER_DISCOVER_EXISTING address TO MUT->ATOMIC#U32;
}
```

The source must have type `ADDRESS`, and the type after `TO` must be a pointer
type. This operation does not construct an object; the object must already
exist at that address.

## Escaping an allocation region

`ADDRESS_LAUNDER_ESCAPE_ALLOC_REGION` exposes an allocator-internal pointer as
an `ADDRESS` while retaining the address's existing provenance relationship:

```quxlang
::storage_address FUNCTION(@storage MUT=>>TYPED_STORAGE(BYTE)): ADDRESS
{
  RETURN ADDRESS_LAUNDER_ESCAPE_ALLOC_REGION storage;
}
```

The operand must be a pointer. This form is useful for allocator metadata and
native interfaces that transport raw addresses.

## Choosing the correct operation

| Intent | Form |
| --- | --- |
| Begin typed authority for newly allocated storage | `BEGIN_ALLOC_REGION address TO pointer_type` |
| End that authority before raw deallocation | `END_ALLOC_REGION pointer` |
| Refer to an already-existing external object | `ADDRESS_LAUNDER_DISCOVER_EXISTING address TO pointer_type` |
| Expose an allocator-internal pointer as an address | `ADDRESS_LAUNDER_ESCAPE_ALLOC_REGION pointer` |
| Reinterpret between explicitly compatible representations | `value AS REINTERPRET Type` |

The laundering operations are unavailable during compile-time evaluation.
They neither allocate storage nor start a payload lifetime. Use
[Allocation regions](allocation-regions.md) for raw allocator storage and
[Typed storage and explicit lifetime](typed-storage-and-lifetime.md) for object
lifetime transitions.
