# Move Semantics

Quxlang represents a consumable object through a `TEMP&` reference. Move
construction is ordinary constructor overload selection with a temporary
source, and `FORWARD(name)` preserves the reference category of a named binding.

## Move constructors

A move constructor accepts `TEMP&` for `OTHER`:

```quxlang
.CONSTRUCTOR FUNCTION(@OTHER TEMP& buffer)
{
  .data <-> OTHER.data;
  .size <-> OTHER.size;
}
```

Constructing from a temporary source can select this overload instead of a
`CONST&` copy constructor. The constructor defines the moved-from state. A
moved-from object remains alive and will still be destroyed; its type must leave
it in a state that satisfies that later destruction.

## `FORWARD`

`FORWARD(name)` accepts the name of a visible local or parameter whose declared
type is a reference. It produces another reference preserving the binding's
current qualifier:

```quxlang
::relay FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

The argument is an identifier, not an arbitrary expression. Forwarding does not
itself construct or move an object. It preserves whether overload resolution
should see a mutable, constant, or temporary reference, allowing a later
constructor or function call to select the corresponding overload.

Using `FORWARD` on a non-reference local, a non-name expression, or a name that
is not visible is ill-formed.

## Movement versus assignment

`destination := source` is assignment and does not implicitly mark a named
source temporary. `VAR destination T := FORWARD(source);` initializes a new
object and can select a move constructor when `source` carries `TEMP&`.

Similarly, returning `FORWARD(parameter)` preserves a forwarding parameter's
category; returning the parameter name without forwarding uses its ordinary
named-expression category.

## Generated movement

Structures may receive generated move construction when their declaration and
members permit it. Arrays move corresponding elements. Unions and variants move
the active payload; unless `NEVER_VALUELESS` applies, the source may become
valueless.

`MOVE_ONLY` suppresses copy-oriented generated operations and communicates that
ownership must transfer. Other `NO_IMPLICIT_*` and fusion `NO_DEFAULT_MOVE`
modifiers control the generated surface described on the relevant type pages.

Move support is demand-driven: defining a type does not require every possible
move path to be instantiated, but a use that selects an unavailable operation
is ill-formed.

## Lifetime and references

`TEMP&` is a reference qualifier, not a new storage duration. It does not extend
the source object's lifetime beyond the ordinary temporary or owning scope.
Storing a pointer or reference taken from a temporary source is valid only when
the referenced object outlives that stored access.

See [References](references.md) for qualifier binding and
[Constructors and Destructors](constructors-and-destructors.md) for lifecycle
selection.
