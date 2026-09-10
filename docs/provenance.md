# Provenance Controlling Operations

## Introduction and Forewarning

Provenance Controlling Operations are operations to be used by memory allocators to control provenance of memory allocations.

Do not use these operations outside of memory allocators. Do not use these operations even in custom containers.
Unless you are writing a memory allocator, you should never adjust pointer provenance.

The ordinary way to write containers is using `TYPED_STORAGE`, `VIRTUAL_STORAGE`, or `ALIGNED_STORAGE`.

Provenance controlling operations are a much lower level tool that can easily introduce undefined behavior and
must be used with extreme caution.

It must be stated before beginning, that provenance manipulating operations are an extreme hazard to the inexperienced
programmer or academic. In particular, provenance manipulating operations operate outside the "pointer-machine" and
"von neumann" models of computation. A formal understanding of the Abstract Machine Memory Model is _absolutely
required_ in order to use these opeartions safely.

To understand the concept of memory provenance, consider a sequence like the following:

```
foo := alloc(16 bytes)
foo[0...16] := inputs
do_calculations(foo)
result := foo[0]
free(foo)
```

In such cases, it would be desirable to avoid using heap allocated memory, instead using stack allocated memory to reduce
overhead. Where the allocator functions perform fixed sequences of operations, this transformation is not allowed.

Initially, this motivates the usage of allocator functions to perform fixed sequences of operations.

But there are other properties of allocator functions which are known, such as the fact that multiple different allocation
regions cannot overlap, and that the memory allocator does not overwrite the content of our allocations, even if it knows
about them.

Namely, given:

```
p1 := alloc(60)
...
p2 := alloc(70)
```

We know that `p2 := alloc(70)` does not overwrite the content of `p1`, and that the returned region is non-overlapping.

To enforce these contracts without requiring the compiler to do complex whole program analysis, in ways that is often not
computationally feasible, we introduce the concept of provenance allocation regions.

## Provenance Regions

The two simplest provenance functions are as follows:

* `PROVENANCE_BEGIN_ALLOC_REGION`
* `PROVENANCE_END_ALLOC_REGION`

This imposes some contract between the programmer and the compiler, which the compiler may use for optimization purposes.
A violation of this contract by the programmer produces undefined behavior.

The `PROVENANCE_BEGIN_ALLOC_REGION` keyword translates an `ADDRESS` to an alloc region storage pointer. Henceforth, it
is forbidden to mutate the contend of the alloc region through any method other than a pointer derived from the return
value of `PROVENANCE_BEGIN_ALLOC_REGION`. Violation of that contract produces undefined behavior. Likewise, the creation
of multiple overlapping alloc regions is a contract violation which produces undefined behavior.

Conceptually, `PROVENANCE_BEGIN_ALLOC_REGION` also destroys any previous data stored in the alloc region. This makes
depending on the content of the initial data in the memory region undefined behavior. The exception to this rule is
if the `INIT_ZERO` or `ASSUME_ZERO` keywords are used with `PROVENANCE_BEGIN_ALLOC_REGION`. Another property of
`PROVENANCE_BEGIN_ALLOC_REGION` is that it allows "consistent" remapping of memory region orders.

`PROVENANCE_BEGIN_ALLOC_REGION` may behave differently if compiled in a "hardened" configuration, for example,
it may be defined to set memory bytes to 0xFA or 0x00 depending on hardening options.

The corresponding keyword is `PROVENANCE_END_ALLOC_REGION`. This has three effects, first it causes any pointer
derived from the original pointer to the alloc region to be _invalidated_. Second, it causes any data content in
the region to become undefined or unspecified, unless a hardening option is used, or another keyword. Finally, it
releases the restraints on the alloc region imposed by `PROVENANCE_BEGIN_ALLOC_REGION`, and allows modifications
to the content of the alloc region by address pointers. It returns the ADDRESS of the alloc region, freed from
restrictions.

Note in Quxlang that invalidated pointers remain totally ordered and consistently ordered, it is only illegal
to derference them.

With regard to reordering, `PROVENANCE_END_ALLOC_REGION` allows "consistent" remapping of memory alloc region orders.
This means specifically, that the `TYPED_STORAGE` pointers need not share the same total ordering as the associated
ADDRESS pointers. However, it **MUST NOT be possible to observe an inconsistency between the address ordering and
the storage ordering**. In practice, this means the compiler can only perform such storage reordering if it can prove
that the storage pointers do not escape, and they are only compared with other known regions which are not _address
convertible_.

Given addresses 1, 5, 10, if allocation regions A, B, and C are allocated from those addresses, the compiler is free
to order the storage pointers in the order B, C, A if it wents to, however, the address ordering must remain consistent
at 1, 5, 10. Ordering must be imposed transisitvely, however, if `PROVENANCE_LAUNDER_DISCOVER_ALLOC_ADDRESS` or 
`PROVENANCE_LAUNDER_DISCOVER_EXISTING_ALLOC` is used, it is possible to observe an inconsistency between the address
ordering and the storage ordering. Any alloc region which can cross the alloc/storage barrier through
`PROVENANCE_LAUNDER_DISCOVER_ALLOC_ADDRESS` or `PROVENANCE_LAUNDER_DISCOVER_EXISTING_ALLOC` cannot be reordered in this
fashion, because the reordering rule does not permit inconsistency of observed address orderings. Likewise, any
escaping or external value cannot be assumed to be safe because escaping values could potentially cross the alloc/storage barrier
through `PROVENANCE_LAUNDER_DISCOVER_ALLOC_ADDRESS` or `PROVENANCE_LAUNDER_DISCOVER_EXISTING_ALLOC`.

`PROVENANCE_LAUNDER_DISCOVER_ALLOC_ADDRESS` allows the discovery of the address of an alloc region from a storage pointer.
Unlike `PROVENANCE_END_ALLOC_REGION`, it does not destroy the content nor does it end restrictions on modifiction of the
associated data.

