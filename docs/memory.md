# Memory Management in Quxlang

Quxlang _Standard Allocators_ can be divided into a three classes based on their interfaces:

* Instance Allocators (of type `T`)
  - `.ALLOC FUNCTION(): ->TYPED_STORAGE(T)`
  - `.DEALLOC FUNCTION(@PTR -> TYPED_STORAGE(T))`
* Multi Allocators (of type `T`)
  - `.MULTI_ALLOC FUNCTION(@COUNT num): =>>TYPED_STORAGE(T)`
  - `.MULTI_DEALLOC FUNCTION(@PTR =>> TYPED_STORAGE(T), @COUNT num)`
* Virtual Allocators (typeless)
  - `.VIRTUAL_ALLOC FUNCTION(@SIZE SZ, @ALIGN SZ): ->VIRTUAL_STORAGE`
  - `.VIRTUAL_DEALLOC FUNCTION(@PTR ->VIRTUAL_STORAGE, @SIZE SZ, @ALIGN SZ)`

Each of these allocator classes differs based on its interface and allowed behaviors.

The `DEFAULT_ALLOCATOR` is a template symbol which when instantiated is an Instance Allocator and an Array Allocator.
It is unspecified whether the instantiated default_allocator is an object, namespace, or singleton etc. 
`DEFAULT_ALLOCATOR#VOID` provides a virtual allocator, and optionally SIZE/ALIGN allocators (not covered in this document). 

## Instance Allocators

Instance allocators provide 2 template functions: `ALLOC` and `DEALLOC`.

The `ALLOC` function takes no arguments and returns an instance pointer to `TYPED_STORAGE(<T>)`.

The `DEALLOC` function takes an instance pointer to `TYPED_STORAGE(<T>)` and deallocates it.

Instance pointers allow drawing from thread local storage using a single increment and compare, thus they are extremely efficient.

Each type `<T>` which is not _oversized_ is mapped to a slab allocator for some compatible _size/align_ values.

Based on configuration or tuning, each slab_allocator has a PER_THREAD pool of some number of slabs. Suppose for example that for
the size 24 align 8 slab allocator we will refer to it as SA24/8 for brevity.

Given some type `foo` with a size of 24 and align of 4, it may be mapped to the SA24/8 slab allocator because not every unique size align combination is _nessecarily_ given its own slab allocator.

The default slab_allocator has a PER_THREAD pool of e.g. 32 allocations at this size bucket for 256 bytes of per thread memory consumed, plus an additional 768 bytes if the allocations themselves are included. Allocation consists of checking if the per thread storage is empty, refilling from a global pool if it is, but otherwise decrementing an alloc counter and returning the stored pointer.

This reduces allocations in the fast-case down to a branch and fast compare.

## Array Allocators

Array Allocators work in many senses like instance allocator, but they are used to allocate and deallocate arrays of _known_ sizes.

Thus, array allocators are pre-configured with a given cell-size and alignment, where the parameter is the number of elements in the array.

`ARRAY_ALLOC(@COUNT num) =>> TYPED_STORAGE(T)`
`ARRAY_DEALLOC(@PTR =>> TYPED_STORAGE(T), @COUNT num)`

Unlike C++ style deallocators, an array allocator must know the size of the array being deallocated. This allows array allocations and deallocations to be quickly rediected to slab allocators in common cases, especially with small arrays.

Array allocators specifiy count in the number of elements, not the number of bytes.

An array allocator can redirect to an "array slab allocator" of the appropriate size.

Unlike an array allocator, an array slab allocator does not carry type information about the type of the elements it allocates for. Therefore many
array allocators for different types of similarly sized objects can share the same array slab allocator.

Because array slab allocators often redirect themselves
to slab allocators, they can be more numerous than slab allocators without incurring additional memory overhead.

For example, an array slab allocator for 1/1 sized objects (common for e.g. strings) can redirect allocations to 8/8 16/8, 32/8 and 64/8 slab allocators in common cases based on the number of elements.

We suggest implementing such an allocator using a comparison which divides the allocation count into "small", or "large/huge" count based on some upper threshold. The small configuration should generally be 8-way classes.

Suggested 1-bit "small" compaction options on 64-byte cache lines:

- 8/8
- 16/16
- 32/32
- 64/64
- 128/128
- 256/256
- 512/256
- 1024/256

For the 2-bit compaction options, we suggest:

- 8/8
- 12/4
- 16/16
- 24/8
- 32/32
- 48/16
- 64/64
- 96/32

The choice of 8-way small comparison was chosen because it can be implemented in exactly 3 comparisons (4 once small/not small branch is included), which typically performs well compared to an indirect load based branching system. The choice between 1 and 2-bit compaction is a speed/ram tradeoff.

Additional compaction options beyond 2-bit are possible, but it's recommended not to use more than 2-bit compaction below 256-byte allocations as the performance overhead can be significant.

One complication arises in the case of array allocators however, in that an 8-way comparison matrix may produce unusual sizes due to unusually sized objects.

For example, given 20-byte objects with 1 byte align, they may be mapped like:

- 1 = 20byte, 24/8 (1)
- 2 = 40byte, 48/16 (2)
- 3 = 60byte, 64/64 (3)
- 4 = 80byte, 96/32 (4)
- 5 = 100byte, 128/64 (5)
- 6 = 120byte, 128/64 (5)
- 7 = 140byte, 192/64 (6)
- 8 = 160byte, 192/64 (6)
- 9 = 180byte, 192/64 (6)
- 10 = 200byte, 256/64 (7)
- 11 = 220byte, 256/64 (7)
- 12 = 240byte, 256/64 (7)
- 13 = 260byte, 384/64 (8)
- 14 = 280byte, 384/64 (8)

Compared to 24/8 array slab allocators:

1 = 24byte, 24/8 (1)
2 = 48byte, 48/16 (2)
3 = 72byte, 96/32 (4)
4 = 96byte, 96/32 (4)
5 = 120byte, 128/64 (5)
6 = 144byte, 192/64 (6)
7 = 168byte, 192/64 (6)
8 = 192byte, 192/64 (6)
9 = 216byte, 256/64 (7)
10 = 240byte, 256/64 (7)
11 = 264byte, 384/64 (8)
12 = 288byte, 384/64 (8)
13 = 312byte, 384/64 (8)
14 = 336byte, 384/64 (8)

As you can see from the above, an 8-way comparison matrix of 20/1 sized objects has small alloc breakpoints at 1, 2, 3, 4, 5, 7, 10, and 13 elements, which are distinct from the 8-way comparison matrix for the 24/8 array slab allocator. As a result, array slab allocators usually should not be rounded up to nearby slab sizes.

Large allocations differ from the small allocations in that we may perform a significantly larger amount of comparisons than 3, such as 4 or 5, up to some arbitrary threshold where the allocation strategy transitions from the slab allocators to the "huge" allocator.

## Virtual Allocator

The virtual allocator differs from the Array Allocator in that it doesn't specialize based on object type, and requires sized-delete much like
the Array Allocator. Unlike the Array Allocator, the Virtual Allocator also requires an runtime provided alignment value.

Virtual Allocators support the introduction of _polymorphic_ object deletion, which can examine the type information for the size and alignment of the object being deleted through runtime type information. Unlike the C/C++ malloc/free interface, the size/alignment information is passed into the virtual deallocate function, which allows the implementing allocator to omit this information from internal allocation headers or redirect to slab allocators.

These functions return a storage pointer of type `->VIRTUAL_STORAGE`. `VIRTUAL_STORAGE` need not only store polymorphic object types, but is mainly intended for these objects as it seems suboptimal to use with other object types.

`VIRTUAL_ALLOC(@SIZE SZ, @ALIGN SZ): ->VIRTUAL_STORAGE`
`VIRTUAL_DEALLOC(@PTR ->VIRTUAL_STORAGE, @SIZE SZ, @ALIGN SZ)`

## Dyanmic Allocators

Dynamic allocators are the main family of allocators which do not return `*_STORAGE` types. Instead _Dynamic Allocators_ operate on `ADDRESS` types.

This functionality gives dynamic allocators the most flexiblity to manipulate memory. Dynamic allocators are non-standard, often used to implement other allocators, and do not have a specific API.

## Alloc Regions

Quxlang has the concept of Allocation Regions, which allows translating between `ADDRESS` and storage types, or to manipulate provenance of `ADDRESS` objects.

In release builds, alloc region operation can be implemented no-ops or allocation provenance annotations for optimizations. In debug builds, alloc regions can be used for address sanitizers.








