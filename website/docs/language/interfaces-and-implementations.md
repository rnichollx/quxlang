# Interfaces and Implementations

An interface declares a runtime-dispatched function surface. An implementation
binds that surface to concrete functions.

## Interface declarations

```quxlang
::integer_source INTERFACE DEFAULTABLE
{
  .read FUNCTION(): I32;
  .fallback FUNCTION(@ARG:value I32): I32
  {
    RETURN value + 100;
  }
}
```

An interface member may be a signature ending in `;` or may include a default
body. Overloaded member names are allowed when their signatures differ.

`DEFAULTABLE` permits a default or `NULL` interface handle. A default method can
still be called on an empty defaultable handle when its body does not require a
concrete implementation slot.

## Implementations

```quxlang
::integer_source_impl IMPLEMENTATION(integer_source)
{
  ::read FUNCTION(): I32
  {
    RETURN 7;
  }

  ::fallback FUNCTION(@ARG:value I32): I32
  {
    RETURN value + 1;
  }
}
```

Implementation functions use nested `::name` declarations matching interface
members. A required signature that is missing or incompatible makes conversion
to the interface invalid.

## Interface handles

```quxlang
VAR source integer_source := integer_source_impl;
ASSERT(source??);
ASSERT(source.read() == 7);
ASSERT(source.fallback(4) == 5);

VAR empty integer_source;
ASSERT(empty?!);
ASSERT(empty.fallback(4) == 104);
```

Handles are copyable values. Assigning an implementation symbol selects that
implementation; assigning `NULL` clears a `DEFAULTABLE` handle. Member calls
dispatch through the current implementation or use the interface default body.

See [Generics](generics.md) for owning and non-owning type erasure of objects.

