# Object Storage and Lifetime

Quxlang distinguishes storage from a live object. A storage value reserves a
location with a size and alignment, while `PLACE AT` starts an object lifetime,
`PUN` accesses the live object, and `DESTROY AT` ends its lifetime.

## Storage types

### `TYPED_STORAGE`

```quxlang
VAR point_slot TYPED_STORAGE(point);
VAR alternative_slot TYPED_STORAGE(I32, point);
```

`TYPED_STORAGE(T...)` authorizes the location for any one of the listed types.
At least one type is required. Its size and alignment accommodate every listed
alternative, but the storage value does not record an active alternative and
does not construct one automatically.

### `ALIGNED_STORAGE`

```quxlang
VAR raw_slot ALIGNED_STORAGE(16, 8);
```

`ALIGNED_STORAGE(size, alignment)` reserves exactly the requested layout. The
size and alignment are part of the type. Placing `T` requires the storage to be
large enough and sufficiently aligned for `T` on the selected target.
Layout-dependent storage is unavailable where the target does not provide the
needed concrete layout.

Storage values may themselves be automatic variables, members, array elements,
or allocator-managed objects. In every case, allocating storage is separate
from starting a payload lifetime.

## Starting a lifetime with `PLACE AT`

The statement and expression grammar shares the same constructor forms:

```quxlang
PLACE AT(location) T;
PLACE AT(location) T := source;
PLACE AT(location) T :(@named value);
PLACE AT(location) T :[first, second];
```

- the bare form default-constructs `T`;
- `:=` uses the ordinary `@OTHER` construction path;
- `:(...)` supplies named and grouped call arguments;
- `:[...]` supplies positional constructor arguments.

`location` must evaluate to a reference to `TYPED_STORAGE` or
`ALIGNED_STORAGE`. `T` must be authorized by typed storage or fit the explicit
raw layout.

As an expression, `PLACE AT` returns a mutable single-object pointer to the
newly live object:

```quxlang
VAR slot TYPED_STORAGE(point);
VAR placed MUT->point := PLACE AT(slot) point:[4, 7];
ASSERT((placed->).x == 4);
```

The statement form discards that pointer but performs the same construction.
Construction must not begin in a location that already contains a live object.

## Accessing the live object with `PUN`

```quxlang
VAR value MUT& point := PUN slot AS point;
value.x := 9;
```

`PUN storage-expression AS T` returns a reference to the live `T` occupying
that storage. Its access qualification follows the storage expression. The
alternative spelling `PUN (storage-expression AS T)` parses equivalently.

`PUN` does not construct, convert, or reinterpret an unrelated object. The
caller must already have started a `T` lifetime at that exact location. The
operation is invalid before placement, after destruction, for a different
active type, or through storage whose allocation has ended.

## Ending a lifetime with `DESTROY AT`

```quxlang
DESTROY AT(slot) point;
```

`DESTROY AT(location) T;` invokes `T`'s destructor and ends the live lifetime.
If the destructor has explicit arguments, supply them with a named argument
list:

```quxlang
DESTROY AT(slot) resource :(@context context);
```

The selected type and location must identify the currently live object. After
destruction, the same storage may be reused by a later `PLACE AT`, including for
another type authorized by `TYPED_STORAGE`.

## Lifetime and allocation ordering

For separately allocated storage, the required order is:

1. allocate the storage;
2. `PLACE AT` each intended object;
3. use the object directly or through `PUN`;
4. `DESTROY AT` every live object;
5. release the storage allocation.

Deallocating storage with a live payload, double destruction, access after
destruction, use after deallocation, and freeing an interior pointer violate
the lifetime or allocation contract. Compile-time allocation checks these
conditions during constexpr execution.

See [Compile-Time Allocation](constexpr-allocation.md) for
`CONSTEXPR_ALLOC` and its multiple-object form, and
[`NEW` and `DELETE`](new-and-delete.md) for the combined
allocator-and-lifetime operation.
