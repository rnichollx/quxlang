# Provenance

In Quxlang provenance affects what data may be _read_ or _written_ through a given pointer.

Comparison of two ADDRESS objects never produces undefined behavior in Quxlang, although
the results may be unspecified-execution-consistent depending on the context of the
comparison.

See: https://discourse.llvm.org/t/rfc-allocator-provenance-model/91106

The goals are:

1. Create an insertion point for annotations related to address sanitizer functionality.
2. Allow inserting `@llvm.provenance.alloc` and `@llvm.provenance.dealloc` for optimizations in release builds.
3. Supporting hardware accelerated bounds checking e.g. CHERI (maybe?).
4. Debug information and annotations where enabled.

Non-goals:

1. Runtime tracking of allocs during normal execution (not including ASAN)
2. Safe or bounds checked allocs on optimized platforms that don't have hardware bounds checking.
3. Running these ALLOC expressions in constexpr VM.

# BEGIN_ALLOC_REGION

Synopsis:

`BEGIN_ALLOC_REGION <expr> TO <storage_instance_pointer_type>`

Casts an `ADDRESS` to a storage pointer while beginning storage provenance in the selected region.

Counterpart:

`END_ALLOC_REGION <storage_pointer>`

Ends an alloc region allocated with `BEGIN_ALLOC_REGION` and returns an `ADDRESS`. The `ADDRESS`
returned by `END_ALLOC_REGION` has the parent provenance.

The behavior is undefined if there is any live object in the allocated region.

# BEGIN_MULTI_ALLOC_REGION

Synopsis:

`BEGIN_MULTI_ALLOC_REGION <expr> SIZE <count> TO <storage_pointer_type>`

Similar to `BEGIN_ALLOC_REGION`, but also includes a count of the number of storage elements included,
to be used for dynamically sized arrays of known storage types.

Counterpart:

`END_MULTI_ALLOC_REGION <expr> SIZE <count>`

Takes a storage array pointer, pops the provenance region, and returns an `ADDRESS`. `SIZE` is 
optional but should be provided if known.

The behavior is undefined if there are live objects in the allocated region.

Additional:

Counterpart:

`RESIZE_MULTI_ALLOC_REGION <expr> COUNT <newcount>`

Changes the alloc size of an existing alloc. The expr must point to the start of the alloc, and the alloc
grows or shrinks as required. If the new size is smaller, and the deallocated portion had any live objects,
then the program behavior is undefined.

# BEGIN_DYNAMIC_ALLOC_REGION

Synopsis:

`BEGIN_DYNAMIC_ALLOC_REGION <expr> SIZE <count>`

Takes an address and returns a _provenance constrained_ `ADDRESS` object.

`END_DYNAMIC_ALLOC_REGION <expr> SIZE <count>`

Recovers a pointer to the parent provenance while ending the provenance constraint of the provided address.

`RESIZE_DYNAMIC_ALLOC_REGION <expr> SIZE <newsize>`

Changes the alloc size of an existing alloc region in-place. The input argument must have provenance
to the allocation to be resized. The expr must point to the start of the alloc, 
and the alloc grows or shrinks based on the value of _newsize_.

If the new size is smaller, and the _deallocated region_ had any live objects,
then the program behavior is undefined. If a _parent allocation_ was resized smaller, and the deallocated
region overlaps with any suballocation, then the behavior is also undefined.

# PARENT_ALLOC_ADDRESS

Synopsis:

`PARENT_ALLOC_ADDRESS <storage_pointer or address>`

`PARENT_ALLOC_ADDRESS` exposes the address of the allocation as an `ADDRESS` associated only with the direct
 parent allocation. It does not carry read or write authority for the child allocation.

This is used, for example, by `realloc`-like functions to obtain the address for size checking
without deallocating the storage, to see if in-place realloc is possible.

# RELOCATE_REGION_OBJECTS

Synopsis:

`RELOCATE_REGION_OBJECTS FROM <alloc1> TO <alloc2> SIZE <byte count>`

Relocates any objects in the `alloc1` region to `alloc2` if any exist.

If at the start of this call, any live object exists in the destination region, the program behavior 
is undefined.

If at the start of this call, any live object exists in the source region and the object is not 
_trivially relocatable_, then the program behavior is undefined.

# Notes:

All of these operations are only allowed in native code, not during constexpr evaluation.

Each `BEGIN_*_ALLOC_REGION` creates a fresh allocation-region identity. Storage pointers 
derived from that operation carry authority for that region. `END_*_ALLOC_REGION` destroys that identity.

Only a `DYNAMIC_ALLOC` can contain sub-allocations.

The `ADDRESS` type supports byte-based arithmetic, but objects of type BYTE cannot alias non-BYTE objects, 
even objects which are BYTE sized, like I8 and U8.


