# `NEW` and `DELETE`

`NEW` obtains storage from `MODULE(RUNTIME)::DEFAULT_ALLOCATOR`, constructs objects in it, and returns a mutable pointer. `COUNT` selects a
runtime-sized array allocation. `DELETE` destroys the objects and returns their
storage to the matching allocator.

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

`NEW [2]I32` returns a pointer to one fixed-array object.
`NEW I32 COUNT(2)` returns `MUT=>>I32`.

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
pointed-to type must not be `VOID`. Constant pointers, references, raw addresses, and garbage-collected pointers
do not satisfy this contract. An array pointer requires `DELETE COUNT(n)`.

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

`NEW` and `DELETE` provide combined allocation and object lifetime management.
[Object Storage and Lifetime](typed-storage-and-lifetime.md) specifies separate
storage and lifetime operations for storage owned separately from the payload.
[Compile-Time Allocation](constexpr-allocation.md) specifies explicit constexpr
allocation.

## Runtime-sized arrays

Runtime-sized allocation and generator construction are supported by the LLVM
backend and constant evaluation.

```quxlang
VAR count SZ := 4;
VAR values MUT=>>I32 := NEW I32 COUNT(count) INDEX(i) DO
{
  INIT := i AS CHECKED I32;
  CONTINUE;
};
DELETE COUNT(count) values;
```

`COUNT(expression)` evaluates its expression once as `SZ`. Its presence selects
array allocation for every count, including zero and one. Elements occupy
contiguous storage and are initialized in ascending index order. Deletion
destroys elements in descending index order. The deletion count, element type,
allocator, and original allocation pointer must match the allocation.

Counted allocation accepts each ordinary constructor initializer. Its initializer
expressions are evaluated separately for each element. A zero count performs
matching zero-count allocation and deallocation operations and invokes no element
generator.

## Element generators

```quxlang
::construct_element FUNCTION(@INITVAL NVALUE[I32], @INDEX:index SZ)
{
  INIT := (index AS CHECKED I32) + 10;
  RETURN;
}

VAR values MUT=>>I32 := NEW I32 COUNT(4) GENERATOR(construct_element);
DELETE COUNT(4) values;
```

`GENERATOR(expression)` evaluates the callable expression once. Every element
call uses that same callable. Overload resolution first supplies `@INITVAL` and
`@INDEX`. It retries with only `@INITVAL` when the indexed call has no viable
overload. An ambiguous indexed call or an error in its selected body remains an
error. The selected generator must return `VOID`.

`DO { statements }` defines an element initialization body with an
`@INITVAL NVALUE[T]` destination. `INDEX(name)` additionally binds the element
index as `@INDEX:name SZ`. The body follows
ordinary lambda capture and exception rules. `INDEX` requires `DO`;
`GENERATOR` and `DO` require `COUNT`. A single allocation accepts exactly one
initialization form.

`INIT` initializes the lambda's destination and execution continues afterward.
An unlabeled `CONTINUE;` outside a nested runtime loop completes the current
element's initialization and executes ordinary exit cleanup. Falling through the
body has the same completion behavior. A `CONTINUE` inside a nested loop targets
that loop; labeled forms target their named loop. `RETURN` and `RETURN_UNEQUAL`
are rejected in the implicit `DO` body. Explicit functions and lambdas passed to
`GENERATOR` retain their ordinary `RETURN` semantics. Every normal completion
path must initialize the destination exactly once. See
[Initialization parameters](functions-and-parameters.md#initialization-parameters).

The completed prefix advances only when an element call returns normally,
including the call's exit cleanup. An exceptional exit destroys the current
completed output and any partially constructed subobjects through ordinary
function cleanup. Allocation cleanup then destroys the completed prefix in
reverse order and releases the allocation.

## Allocators and pointer slots

`WITH(allocator)` selects a custom allocator. Counted allocation calls its typed
`ALLOCATE#T(@COUNT n)` operation. Counted deletion calls
`DEALLOCATE#T(@PTR storage, @COUNT n)`. The pointer identifies
`MUT=>>TYPED_STORAGE(T)`. The default allocator supplies `MULTI_ALLOC` and
`MULTI_DEALLOC` for these operations.

```quxlang
VAR values MUT=>>I32;
NEW COUNT(3) TO(values) := 7;
DELETE FROM(values) COUNT(3);
```

`TO(slot)` infers the element type from the pointer slot and publishes the pointer
only after complete construction succeeds. `FROM(slot)` clears the slot after
successful deletion. `OPTIONAL` preserves the corresponding scalar operation's
skipping behavior. `COUNT`, `WITH`, `TO`, and `OPTIONAL` allocation clauses may
precede or follow an explicit element type. `FROM` as an allocation initializer
continues to select construction from a source expression.

## Evaluation order

A `TO` destination is evaluated first, and an optional-operation check follows.
When allocation proceeds, the allocator expression is evaluated, followed by the
count and explicit generator expressions. Allocation then obtains storage, and
element calls run in ascending index order. A constructor initializer's source
and argument expressions execute inside each element call. The completed pointer
is published to the destination after the final call returns normally.

Deletion evaluates its pointer or `FROM` slot first. An optional-operation check
precedes evaluation of the allocator and count. Destruction runs in reverse index
order, followed by deallocation and clearing of a `FROM` slot. A skipped optional
operation evaluates neither its allocator nor its count or initializer.
