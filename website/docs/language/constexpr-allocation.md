# Compile-Time Allocation

Quxlang's compile-time allocator intrinsics reserve storage while a function is
being evaluated as `CONSTEXPR`. They expose storage, not a live object: use
`PLACE AT` to begin an object lifetime and `DESTROY AT` before deallocation.

## One storage element

`CONSTEXPR_ALLOC` and `CONSTEXPR_DEALLOC` take the stored type as their template
argument:

```quxlang
::allocated_integer STATIC_TEST
{
  VAR storage ->TYPED_STORAGE(I32) := CONSTEXPR_ALLOC#I32();
  VAR value ->I32 := PLACE AT(storage->) I32 := 123;

  ASSERT(value-> == 123);

  DESTROY AT(storage->) I32;
  CONSTEXPR_DEALLOC#I32(% [storage]);
}
```

Explicit size and alignment are available when the eventual stored type is not
part of the allocation interface:

```quxlang
VAR storage ->ALIGNED_STORAGE(8, 8) :=
  CONSTEXPR_ALLOC#(@SIZE 8, @ALIGN 8)();

CONSTEXPR_DEALLOC#(@SIZE 8, @ALIGN 8)(% [storage]);
```

## Multiple storage elements

The multiple-element forms use an array pointer and require the allocation
count at both ends of the lifetime:

```quxlang
::allocated_bytes STATIC_TEST
{
  VAR count SZ := 3;
  VAR storage =>>TYPED_STORAGE(BYTE) :=
    CONSTEXPR_ALLOC_MULTIPLE#BYTE(% [count]);

  PLACE AT(storage->) BYTE := 7;
  PLACE AT(storage[1]) BYTE := 9;
  PLACE AT((storage + 2)->) BYTE := 11;

  ASSERT((PUN storage[1] AS BYTE) == 9);

  DESTROY AT((storage + 2)->) BYTE;
  DESTROY AT(storage[1]) BYTE;
  DESTROY AT(storage->) BYTE;
  CONSTEXPR_DEALLOC_MULTIPLE#BYTE(% [storage, count]);
}
```

The corresponding explicit-layout form is:

```quxlang
VAR storage =>>ALIGNED_STORAGE(size, align) :=
  CONSTEXPR_ALLOC_MULTIPLE#(@SIZE size, @ALIGN align)(% [count]);

CONSTEXPR_DEALLOC_MULTIPLE#(@SIZE size, @ALIGN align)(% [storage, count]);
```

## Checked lifetime rules

Compile-time evaluation rejects invalid allocation use, including:

- freeing the same allocation twice;
- freeing an interior pointer rather than the allocation start;
- supplying the wrong count to `CONSTEXPR_DEALLOC_MULTIPLE`;
- deallocating storage that still contains a live object; and
- using storage or a pointer after its allocation was released.

A zero-count multiple allocation is valid, but it must still be paired with the
matching deallocation call.

These intrinsics are the compile-time side of the runtime allocator interface.
See [Allocation regions](allocation-regions.md) for turning allocator-provided
`ADDRESS` values into storage pointers, and
[Typed storage and explicit lifetime](typed-storage-and-lifetime.md) for
`PLACE AT`, `PUN`, and `DESTROY AT`.
