# Overview of NEW and DELETE

`NEW` combines allocation with object construction through the runtime's
default allocator path.

```quxlang
VAR default_value MUT->point := NEW point;
VAR copied_value MUT->point := NEW point FROM default_value->;
VAR named_value MUT->point := NEW point :(@x 5, @y 9);
VAR positional_value MUT->point := NEW point :[5, 9];
VAR fixed_array MUT->[2]I32 := NEW [2]I32 :[53, 59];
```

## Initializers

- `NEW T` default-constructs one `T`.
- `NEW T FROM expression` constructs from the ordinary source category.
- `NEW T FROM MODE expression` selects `EXPLICIT`, `REINTERPRET`, `PARTIAL`,
  `ASSUME`, `CHECKED`, or `APPROXIMATE` construction.
- `NEW T :(...)` supplies named arguments and positional groups.
- `NEW T :[...]` supplies positional constructor arguments.

`NEW [N]T` allocates one fixed-array object. It does not return a dynamically
sized multi-object pointer.

## Deallocation

```quxlang
DELETE fixed_array;
DELETE positional_value;
DELETE named_value;
DELETE copied_value;
DELETE default_value;
```

`DELETE` destroys the object and releases its allocation. It accepts a mutable
instance pointer. Constant pointers and `=>>T` multi-object pointers are
rejected because they do not carry the required single-object deletion
contract.

The pointer must identify the matching live allocation and must not be used
after deletion.

For storage owned separately from its payload, use
[Typed storage and explicit lifetime](typed-storage-and-lifetime.md).

## Complete technical rules

See the [NEW and DELETE Reference](../../reference/new-and-delete.md) for the complete
language rules, constraints, and technical edge cases.
