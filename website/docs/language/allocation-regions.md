# Allocation Regions

Low-level allocators commonly return an `ADDRESS`, while object construction
needs a typed storage pointer. Allocation-region expressions state where that
transition begins and ends.

## One storage element

`BEGIN_ALLOC_REGION` converts an allocator-provided address to a storage
pointer. `END_ALLOC_REGION` returns the parent `ADDRESS` for deallocation:

```quxlang
::allocate_slot FUNCTION(@address ADDRESS): ->TYPED_STORAGE(I32)
{
  RETURN BEGIN_ALLOC_REGION address TO ->TYPED_STORAGE(I32);
}

::release_slot FUNCTION(@storage ->TYPED_STORAGE(I32)): ADDRESS
{
  RETURN END_ALLOC_REGION storage;
}
```

The target of `BEGIN_ALLOC_REGION` is written after `TO` and must be the pointer
type that describes the allocated storage. Any live payload must be destroyed
before `END_ALLOC_REGION`.

## Multiple storage elements

The multiple-element forms carry the number of storage elements:

```quxlang
::allocate_slots FUNCTION(
  @address ADDRESS,
  @count SZ
): =>>TYPED_STORAGE(BYTE)
{
  RETURN BEGIN_MULTI_ALLOC_REGION address SIZE count
    TO =>>TYPED_STORAGE(BYTE);
}

::release_slots FUNCTION(
  @storage =>>TYPED_STORAGE(BYTE),
  @count SZ
): ADDRESS
{
  RETURN END_MULTI_ALLOC_REGION storage SIZE count;
}
```

The `SIZE count` clause on `END_MULTI_ALLOC_REGION` is optional in the grammar,
but allocator code should provide it whenever the count is known. The storage
pointer must identify the beginning of the allocation.

## Runtime allocator pattern

A typed allocator separates raw allocation, region establishment, object
lifetime, and raw deallocation:

```quxlang
VAR address ADDRESS := alloc_native(
  @size SIZEOF(I32),
  @align ALIGNOF(I32)
);
VAR storage ->TYPED_STORAGE(I32) :=
  BEGIN_ALLOC_REGION address TO ->TYPED_STORAGE(I32);
VAR value ->I32 := PLACE AT(storage->) I32 := 42;

ASSERT(value-> == 42);

DESTROY AT(storage->) I32;
address := END_ALLOC_REGION storage;
dealloc_native(@address address, @size SIZEOF(I32), @align ALIGNOF(I32));
```

Allocation regions grant storage authority; they do not start or end the
payload object's lifetime. `PLACE AT` and `DESTROY AT` remain separate and
explicit.

## Managed-runtime object storage

The JVM target uses managed object storage rather than converting a raw
`ADDRESS` into a native allocation region. Runtime allocator code can request
one or several typed storage cells directly:

```quxlang
STATIC_IF(ARCH_IS_JVM)
{
  VAR one ->TYPED_STORAGE(I32) :=
    JVM_ALLOCATE_OBJECT_STORAGE#I32();
  JVM_DEALLOCATE_OBJECT_STORAGE#I32(% [one]);

  VAR count SZ := 8;
  VAR many =>>TYPED_STORAGE(BYTE) :=
    JVM_ALLOCATE_OBJECT_STORAGE#BYTE(% [count]);
  JVM_DEALLOCATE_OBJECT_STORAGE#BYTE(% [many, count]);
}
```

These builtins are available only for a JVM target. They return storage, not
live objects, so construction and destruction remain separate lifetime
operations. They are the managed counterpart to the native raw-address region
path and are not compile-time allocation operations.

## Provisional region operations

The current grammar also accepts these lower-level forms:

```text
RESIZE_MULTI_ALLOC_REGION pointer COUNT new_count
BEGIN_DYNAMIC_ALLOC_REGION address SIZE byte_count
END_DYNAMIC_ALLOC_REGION address SIZE byte_count
RESIZE_DYNAMIC_ALLOC_REGION address SIZE new_byte_count
PARENT_ALLOC_ADDRESS pointer_or_address
RELOCATE_REGION_OBJECTS FROM source TO destination SIZE byte_count
```

Their complete provenance, resize, and relocation effects are not yet enforced
by every backend. They are listed here so the accepted syntax and its status are
clear, but portable code should not depend on the unfinished effects.

Allocation-region operations are for native runtime allocation paths, not
compile-time allocation. See [Compile-time allocation](constexpr-allocation.md)
for `CONSTEXPR_ALLOC` and its matching deallocation forms, and
[Runtime module contracts](../toolchain/runtime-module-contracts.md) for the
allocator surface consumed by `NEW` and `DELETE`.
