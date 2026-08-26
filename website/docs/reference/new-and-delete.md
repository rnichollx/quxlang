# `NEW` and `DELETE`

`NEW` obtains storage from `MODULE(RUNTIME)::DEFAULT_ALLOCATOR`, constructs one
object in it, and returns a mutable single-object pointer. `DELETE` destroys the
object behind such a pointer and returns the underlying storage to the default
allocator.

## Result type

For an object type `T`, `NEW T` has type `MUT->T`:

```quxlang
VAR value MUT->point := NEW point;
```

`VOID` cannot be constructed. A fixed array is one object whose array bound is
part of its type:

```quxlang
VAR values MUT->[2]I32 := NEW [2]I32 :[53, 59];
ASSERT((values->)[1] == 59);
```

This is not a dynamic allocation of two independent `I32` objects and does not
produce `MUT=>>I32`.

## Default construction

The bare form selects the default constructor:

```quxlang
VAR value MUT->point := NEW point;
```

The allocation occurs before construction. If `point` has no callable default
constructor, the expression is rejected.

## Construction from one source

`FROM` passes the source through a named constructor category:

```quxlang
VAR copy MUT->point := NEW point FROM value->;
VAR explicit MUT->point := NEW point FROM EXPLICIT encoded;
VAR checked MUT->point := NEW point FROM CHECKED wide_value;
```

Without a mode, the source is bound as `@OTHER`. With a mode, it is bound to the
corresponding constructor parameter:

| Form | Constructor argument |
| --- | --- |
| `FROM expression` | `@OTHER` |
| `FROM EXPLICIT expression` | `@EXPLICIT` |
| `FROM REINTERPRET expression` | `@REINTERPRET` |
| `FROM PARTIAL expression` | `@PARTIAL` |
| `FROM ASSUME expression` | `@ASSUME` |
| `FROM CHECKED expression` | `@CHECKED` |
| `FROM APPROXIMATE expression` | `@APPROXIMATE` |

The selected constructor must accept that category and source type. The mode
does not bypass overload resolution or the semantic contract of the category.

## Named and positional constructor arguments

`:(...)` uses the ordinary call-argument grammar, while `:[...]` supplies a
positional sequence:

```quxlang
VAR named MUT->point := NEW point :(@x 5, @y 9);
VAR positional MUT->point := NEW point :[5, 9];
```

Named and grouped arguments behave as they do for direct construction. See
[Call Arguments](call-arguments.md) and
[Constructors and Destructors](constructors-and-destructors.md).

## Deletion

```quxlang
DELETE positional;
DELETE named;
DELETE values;
```

`DELETE expression` requires a mutable instance pointer. The
pointed-to type must not be `VOID`. Constant pointers, references, raw
addresses, garbage-collected pointers, and `=>>T` multi-object pointers do not
satisfy this contract.

Deletion performs these operations in order:

1. recover the storage underlying the object pointer;
2. invoke the pointed-to type's destructor;
3. end the object lifetime;
4. deallocate the recovered storage through the default allocator.

For an instance pointer to a fixed array, destruction applies to the one array
object and its elements before its one allocation is released.

## Polymorphic deletion

A `POLYMORPHIC` or `VIRTUAL_POLYMORPHIC` struct has a virtual destructor unless
its `.DESTRUCTOR` is tagged `NONVIRTUAL`.

When the destructor is virtual, `DELETE` first saves the complete allocation
information obtained through the object's runtime descriptor. It then invokes
the most-derived destructor and deallocates the saved storage. This permits
deletion through a base pointer.

When the destructor is `NONVIRTUAL`, deletion uses the static type and static
allocation dimensions without checking the runtime type. The pointer must
identify a complete object of exactly that static type; violating this
precondition is undefined behavior.

See [Inheritance](inheritance.md) for the destructor policy and hierarchy
rules.

!!! warning "JVM backend"
    Polymorphic allocation, virtual destruction, and inheritance allocation-info
    recovery are not implemented by the Cortado JVM backend.

The pointer must identify the live object associated with the matching default
allocator allocation. Deleting twice, deleting an interior or unrelated
pointer, or reading through any alias after deletion violates the allocation
and lifetime contract. `DELETE` does not clear other copies of the pointer.

## Relation to explicit storage

`NEW` and `DELETE` are the combined ownership path. Use
[Object Storage and Lifetime](typed-storage-and-lifetime.md) when storage is
owned separately from the payload, [Compile-Time Allocation](constexpr-allocation.md)
for explicit constexpr allocation.
