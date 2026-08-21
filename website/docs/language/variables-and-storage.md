# Variables and Storage Duration

Quxlang distinguishes mutable objects, compile-time constants, static-expansion
state, and per-thread globals.

```quxlang
::mutable_global VAR I32 := 8;
::compile_time_value STATIC I32 := 4;
::per_thread_counter PER_THREAD VAR I32 := 0;

::declarations FUNCTION()
{
  VAR defaulted I32;
  VAR initialized I32 := 12;
  VAR unspecified I32 := UNSPECIFIED;
  STATIC local_constant I32 := 7;
  STATIC_VAR expansion_counter U64 := 0;
}
```

## `VAR`

`VAR` creates mutable storage. Omitting its initializer default-constructs the
declared type; for an integer, that produces zero. `:= expression`, `:(...)`,
and `:[...]` select the initialization forms described on
[Arrays and construction](arrays-and-construction.md).

## `STATIC`

`STATIC` declares a compile-time constant object. It may appear at declaration
scope or inside a function. A global static object's type must support one of
Quxlang's materialization contracts; see
[Static objects and materialization](static-objects-and-materialization.md).

## `STATIC_VAR`

Function-local `STATIC_VAR` is mutable only during static expansion. Runtime
code observes its final expanded value through `SNAPSHOT(name)`:

```quxlang
::expanded_count FUNCTION(): U64
{
  STATIC_VAR count U64 := 0;
  STATIC_WHILE(count < 3)
  {
    STATIC_EVAL count++;
  }
  RETURN SNAPSHOT(count);
}
```

`STATIC_VAR` is not a global declaration kind.

## `PER_THREAD VAR`

`PER_THREAD VAR` is a global declaration. Every thread receives an independent
instance, including independent initialization and destruction for nontrivial
types. It cannot be used for a member or function-local declaration.

## Reserved variable tags

The parser currently recognizes `CONSTEXPR_READABLE` and
`CONSTEXPR_READWRITE` after `VAR`, but no current semantic access contract is
attached to those tags. They are reserved syntax rather than usable storage
qualifiers.

See [Static evaluation](static-evaluation.md) and
[Concurrency and per-thread storage](concurrency-and-per-thread-storage.md).
