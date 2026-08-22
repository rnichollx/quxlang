# Overview of `STATIC` Compile-Time Constants

`STATIC` declares a read-only value whose initializer runs during compilation.
At global scope, the compiler also needs a supported way to represent that
value in the generated program.

## Declare a direct constant

```quxlang
::point STRUCT
{
  .x VAR I32;
  .y VAR I32;
}

::origin_point STATIC point := point();
::answer STATIC I32 := 42;
::answer_pointer STATIC CONST->I32 := answer<-;
```

Eligible scalar values, pointers, arrays, and trivially destructible structures
can be emitted directly. An empty Quxlang structure remains size zero; static
eligibility does not add a synthetic byte.

## Require direct static representation

```quxlang
::offset STRUCT ANTESTATAL
{
  .dx VAR I32;
  .dy VAR I32;
}
```

`ANTESTATAL` makes direct static representation part of the type's contract.
Compilation fails if a field or nontrivial destructor makes that contract
impossible. `NONSTATIC` states the opposite and rejects `STATIC` objects of the
marked structure type.

## Use serialized static representation

```quxlang
::saved_pair STRUCT SERIALOID
{
  .first VAR BYTE;
  .second VAR BOOL;
}

::saved_default STATIC saved_pair := saved_pair();
```

`SERIALOID` uses serialization and deserialization instead of direct object
representation. Quxlang can generate field-wise operations when every field
supports the protocol.

Function-local `STATIC` is also read-only. `STATIC_VAR` is different: it is
mutable during static expansion and runtime code observes it through
`SNAPSHOT`.

## Complete technical rules

See the [`STATIC` Compile-Time Constants Reference](../../reference/static-compile-time-constants.md)
for all eligible categories, dependency tracking, reconstruction order, and the
rules for `ANTESTATAL`, `SERIALOID`, and `NONSTATIC`.
