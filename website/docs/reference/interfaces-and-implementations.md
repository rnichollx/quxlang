# Interfaces and Implementations

An `INTERFACE` declares a runtime-dispatched function surface. A separate
`IMPLEMENTATION` names the concrete functions used for one interface handle.
Interface values carry implementation identity, not an owned user object.

## Interface declarations

```quxlang
::integer_source INTERFACE
{
  .read FUNCTION(): I32;
  .offset FUNCTION(@ARG I32): I32;
}
```

Every interface entry uses member syntax (`.name`) and `FUNCTION`. Static
`::name` entries are rejected. A signature-only entry ends in `;`; it describes
a required dispatch slot.

Parameters, return types, receiver forms, overloads, and procedure
delegates follow the ordinary function rules. A signature-only entry declares
its public argument names but cannot give one argument a different body-local
name, because it has no body. Implementations may choose their own local names.

Comparison operator entries follow the same restriction as struct operators:
interfaces declare `OPERATOR==` or `OPERATOR<=>`, never the synthesized
`!=`, `<`, `>`, `<=`, or `>=` names.

## Default method bodies

An interface entry may provide a body:

```quxlang
::integer_source INTERFACE
{
  .read FUNCTION(): I32;

  .offset FUNCTION(@ARG:value I32): I32
  {
    RETURN value + 100;
  }
}
```

The body is the slot's default implementation. A concrete implementation may
override it with a matching function. A default body may use body-local
parameter names because it is itself executable code.

If a default method calls another interface member, that call still requires a
concrete slot at execution. A body that depends only on its parameters can run
without one when the interface type is `DEFAULTABLE`.

## `DEFAULTABLE`

```quxlang
::optional_source INTERFACE DEFAULTABLE
{
  .read FUNCTION(): I32;
  .fallback FUNCTION(@ARG:value I32): I32
  {
    RETURN value + 100;
  }
}
```

`DEFAULTABLE` adds an empty handle state. It permits default construction and
construction or assignment from `NULL`:

```quxlang
VAR source optional_source;
ASSERT(source?!);
ASSERT(source.fallback(4) == 104);

source := NULL;
ASSERT(source?!);
```

`source??` means that a concrete implementation is present; `source?!` means
the handle is empty. An interface without `DEFAULTABLE` cannot be
default-constructed or initialized from `NULL`.

Calling a required dispatch slot needs a concrete implementation. Only a
suitable default body can execute on an empty handle.

## Implementation declarations

```quxlang
::integer_source_impl IMPLEMENTATION(integer_source)
{
  ::read FUNCTION(): I32
  {
    RETURN 7;
  }

  ::offset FUNCTION(@ARG:value I32): I32
  {
    RETURN value + 1;
  }
}
```

The parenthesized type must be an interface. Functions inside the body are
nested declarations and therefore use `::name`. A concrete function is matched
to an interface slot by member name and callable signature. Its local parameter
names may differ, but its public argument names, parameter types, receiver
contract, and result must satisfy the slot.

Overloaded interface names create distinct slots. An implementation supplies
the matching overloads independently. Missing or incompatible required slots
make conversion from that implementation invalid. A slot with a default body
does not require an override.

The implementation body may contain declarations beyond the exposed surface;
only functions matching interface slots are callable through the interface
handle.

## Constructing and assigning handles

An implementation declaration is the value used to select its dispatch table:

```quxlang
VAR source optional_source := integer_source_impl;
ASSERT(source??);
ASSERT(source.read() == 7);

VAR copy optional_source := source;
ASSERT(copy.read() == 7);
```

Interface handles are copyable and assignable values. Copying preserves the
selected implementation identity. Assigning another implementation retargets
subsequent calls; assigning `NULL` clears a `DEFAULTABLE` handle.

Interface values can also be `STATIC` when their implementation and retained
dependencies satisfy the static-value contract.

## Dispatch and access control

A call first resolves the interface overload using the ordinary argument rules,
then dispatches the selected slot through the handle's current implementation.
Default bodies and implementation functions obey the same privacy rules as
other declarations; interface entries can be individually `PRIVATE`, including
through a `PRIVATE { ... }` block.

Interfaces differ from [Generics](generics.md): an interface handle selects a
declared implementation surface, while an owning `GENERIC` stores and manages a
concrete object and `GENERIC_REF` refers to an existing object structurally.
