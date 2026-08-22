# Generics

Quxlang generics provide structural type erasure. A `GENERIC_REF` refers to an
existing concrete object without owning it; a `GENERIC` stores and owns a
concrete object. Both expose only the operations declared in their generic
surface.

## Operation declarations

```quxlang
::counter_ref GENERIC_REF
{
  .add FUNCTION(@ARG I32) MUT: I32;
  .read FUNCTION() CONST: I32;
}
```

Every operation is a member function with exactly one receiver. The compact
suffix is equivalent to an explicit `@THIS` parameter:

```quxlang
.read FUNCTION(@THIS CONST& THISTYPE): I32;
.add FUNCTION(@THIS MUT& THISTYPE, @ARG I32): I32;
```

The formal declaration retains `THISTYPE`. When a concrete object is bound, the
compiler instantiates the same function contract with that object's type. A
receiver must be a reference to `THISTYPE`; missing receivers, multiple
receivers, or a non-reference receiver are rejected.

Other parameters, their API names, return types, overloads, privacy, and
operators are matched as ordinary function declarations. The concrete type
must provide a callable member for every operation in the effective surface.

## `GENERIC_REF`

`GENERIC_REF` stores a non-owning erased pointer and the dispatch surface for
the referenced object's concrete type:

```quxlang
VAR counter concrete_counter := 4;
VAR view counter_ref := counter;

view.add(@ARG 3);
ASSERT(counter.value == 7);
ASSERT(view.read() == 7);
```

A mutable generic reference can bind only with the access needed by its
operations. Calls through it affect the original object. Copying the generic
reference copies the reference, not the concrete object. The referenced object
must remain alive and at the same location for the entire use of every copied
generic reference.

`GENERIC_REF CONST` stores only constant access:

```quxlang
::readable_ref GENERIC_REF CONST
{
  .read FUNCTION() CONST: I32;
}
```

Every operation of a const generic reference must accept `CONST& THISTYPE`; a
mutable receiver in that surface is rejected.

## Owning `GENERIC`

```quxlang
::counter_value GENERIC
{
  .add FUNCTION(@ARG I32) MUT: I32;
  .read FUNCTION() CONST: I32;
}

VAR source concrete_counter := 10;
VAR owned counter_value := source;
owned.add(@ARG 2);

ASSERT(owned.read() == 12);
ASSERT(source.value == 10);
```

Construction allocates owned erased storage and copy- or move-constructs the
concrete value into it. Mutation through the generic therefore affects the
stored copy, not the source used for copy construction. Destruction dispatches
to the concrete destructor and releases the owned storage.

By default, copying an owning generic deep-copies its current concrete value.
Moving transfers the owned value. `GENERIC MOVE_ONLY` suppresses the generated
copy surface:

```quxlang
::task GENERIC MOVE_ONLY
{
  .run FUNCTION() MUT;
}
```

A generic has no independent default concrete type; construction must provide
a concrete value or another compatible generic value.

## Comparison and concrete type identity

Generics are comparable by default. Equality and three-way comparison first
account for the stored concrete type. Values with different concrete types are
not equal and receive a non-equal type order. Values with the same concrete
type dispatch to that type's equality or ordering operation.

```quxlang
ASSERT(first == copied);
ASSERT((first <=> copied) == ORDER::EQUAL);
```

`GENERIC INCOMPARABLE` removes the generated `OPERATOR==` and `OPERATOR<=>`
surface. `CURRENT_TYPE()` exposes the current concrete type as a `TYPE_INDEX`
when code needs to inspect erased type identity explicitly.

`MOVE_ONLY` and `INCOMPARABLE` may also appear on generic references, although
their non-owning nature remains unchanged.

## Reusing surfaces with `IMPLEMENTS`

`IMPLEMENTS` imports another generic surface:

```quxlang
::extended_counter GENERIC
{
  IMPLEMENTS counter_ref;
  .reset FUNCTION() MUT;
}
```

All `IMPLEMENTS` entries precede locally declared operations. Inclusion is
transitive; inherited and local operations together form the required surface.
The target must itself be a `GENERIC` or `GENERIC_REF`.

The following compatibility rules preserve the declaring type's guarantees:

- `GENERIC_REF` cannot implement an owning `GENERIC`;
- a `MOVE_ONLY` generic cannot implement a copyable generic;
- an `INCOMPARABLE` generic cannot implement a comparable generic;
- cyclic `IMPLEMENTS` graphs are rejected.

An owning generic may implement a generic-reference surface, and another owning
generic may reuse an owning surface when its copy and comparison guarantees are
compatible. `IMPLEMENTS` includes a contract; it does not bind a concrete type
or declare inheritance between user structs.

## Representation boundary

Generic dispatch storage, erased pointers, lifecycle slots, and comparison
slots are compiler-managed. Source code interacts through the declared
operations, generated lifecycle, comparison operators, and `CURRENT_TYPE()`;
it cannot depend on the internal field layout.

See [Interfaces and Implementations](interfaces-and-implementations.md) for
nominal implementation tables and [Functions](functions-and-parameters.md)
for receiver and parameter rules.
