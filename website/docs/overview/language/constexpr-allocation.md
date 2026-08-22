# Overview of Compile-Time Allocation

Compile-time allocation reserves temporary storage while Quxlang evaluates a
`CONSTEXPR` computation. Allocation creates storage, not an object: construct
the value with `PLACE AT`, destroy it with `DESTROY AT`, and then release the
storage.

## Allocate one object

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

The stored type is the template argument. Use the explicit `@SIZE` and `@ALIGN`
form when an allocator should work only with layout:

```quxlang
VAR storage ->ALIGNED_STORAGE(8, 8) :=
  CONSTEXPR_ALLOC#(@SIZE 8, @ALIGN 8)();
CONSTEXPR_DEALLOC#(@SIZE 8, @ALIGN 8)(% [storage]);
```

## Allocate several elements

```quxlang
VAR count SZ := 3;
VAR storage =>>TYPED_STORAGE(BYTE) :=
  CONSTEXPR_ALLOC_MULTIPLE#BYTE(% [count]);

PLACE AT(storage->) BYTE := 7;
PLACE AT(storage[1]) BYTE := 9;

DESTROY AT(storage[1]) BYTE;
DESTROY AT(storage->) BYTE;
CONSTEXPR_DEALLOC_MULTIPLE#BYTE(% [storage, count]);
```

The multiple-element deallocation repeats the original count. Compile-time
evaluation detects mismatched counts, double frees, interior-pointer frees,
live objects left in storage, and use after release.

## Complete technical rules

See the [Compile-Time Allocation Reference](../../reference/constexpr-allocation.md)
for typed and explicit-layout signatures, zero-count behavior, and all checked
lifetime rules.
