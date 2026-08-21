# Typed Storage and Explicit Lifetime

Storage and object lifetime are separate. A storage object can reserve bytes
without making an object of the eventual payload type live.

## Storage types

```quxlang
VAR typed TYPED_STORAGE(point);
VAR alternatives TYPED_STORAGE(I32, point);
VAR raw ALIGNED_STORAGE(16, 8);
```

- `TYPED_STORAGE(T...)` reserves storage authorized for one of the listed
  types.
- `ALIGNED_STORAGE(size, alignment)` reserves untyped storage with an explicit
  layout.
- Layout-dependent storage is unavailable when the target does not provide the
  required static layout.

Neither declaration starts a payload lifetime.

## Start, access, and end a lifetime

```quxlang
VAR storage TYPED_STORAGE(point);
VAR placed ->point := PLACE AT(storage) point:[4, 7];

ASSERT((PUN storage AS point).x == 4);

DESTROY AT(storage) point;
```

- `PLACE AT(location) T` default-constructs `T` in the storage.
- `PLACE AT(location) T := expression` constructs from one expression.
- `PLACE AT(location) T :(...)` and `:[...]` pass constructor arguments.
- `PUN storage AS T` accesses the live `T` occupying authorized storage.
- `DESTROY AT(location) T` ends the live object's lifetime.

`PLACE AT` can be used as an expression returning a pointer to the new object,
or as a statement when the pointer is not needed.

Access before `PLACE AT`, access as the wrong type, access after `DESTROY AT`,
and storage release while a payload remains live violate the storage/lifetime
contract.

See [`NEW` and `DELETE`](new-and-delete.md) for allocator-backed lifetime.

