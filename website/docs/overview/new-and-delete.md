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

`NEW [N]T` allocates one fixed-array object. `COUNT` allocates a runtime-sized
sequence of elements:

```quxlang
VAR values MUT=>>I32 := NEW I32 COUNT(4) INDEX(i) DO
{
  INIT := (i AS CHECKED I32) + 10;
};
DELETE COUNT(4) values;
```

Each `INIT` constructs one element directly in its destination. Execution
continues after `INIT`. Fallthrough or an unlabeled `CONTINUE;` outside a nested
loop completes the element initialization. `GENERATOR(callable)` reuses an
explicit function, lambda, or callable object with ordinary `RETURN` semantics.
A generator accepts `@INITVAL NVALUE[T]` and optionally `@INDEX SZ`.
If construction fails, completed elements are destroyed in reverse order and
the allocation is released.

## Deallocation

```quxlang
DELETE fixed_array;
DELETE positional_value;
DELETE named_value;
DELETE copied_value;
DELETE default_value;
```

`DELETE` destroys the object and releases its allocation. It accepts a mutable
instance pointer, or a mutable array pointer with a matching `COUNT`.
Counted deletion requires the original pointer, element type, allocator, and
count.

The pointer must identify the matching live allocation and must not be used
after deletion.

For a polymorphic struct with a virtual destructor, `DELETE` through a base
pointer destroys the complete runtime object. A destructor tagged `NONVIRTUAL`
uses an unchecked exact-type deletion contract. See
[Inheritance](inheritance.md). Inheritance-based allocation and deletion are
not implemented by the JVM backend.

For storage owned separately from its payload, use
[Typed storage and explicit lifetime](typed-storage-and-lifetime.md).

## Reference

See the [NEW and DELETE Reference](../reference/new-and-delete.md) for the complete
language rules, constraints, and technical edge cases.
