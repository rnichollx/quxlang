# Overview of Conversions

Conversions change a value's type. Quxlang requires each potentially lossy or
unchecked conversion to state its semantics explicitly. The conversion mode
keeps truncation, runtime validation, assumptions, and representation changes
visible in source.

## Conversion modes

```quxlang
VAR wide I64 := 300;
VAR narrowed I8 := wide AS PARTIAL I8;
VAR checked I32 := wide AS CHECKED I32;
VAR assumed I32 := wide AS ASSUME I32;
VAR approximate F32 := 0.4 AS APPROXIMATE F32;
```

- `PARTIAL` permits loss of part of a representation, such as discarded high
  integer bits.
- `CHECKED` validates the conversion and fails if the value is invalid.
- `ASSUME` makes validity a program precondition.
- `APPROXIMATE` permits an inexact numeric result.

Plain `AS Type` performs an ordinary explicit conversion. `AS EXPLICIT Type`
selects a user-defined explicit conversion category.

## Representation reinterpretation

```quxlang
VAR pointer CONST->I32 := value<-;
VAR erased CONST->VOID := pointer AS REINTERPRET CONST->VOID;
VAR restored CONST->I32 := erased AS REINTERPRET CONST->I32;
```

`REINTERPRET` is narrow permission for representation-level paths supported by
the type system. It does not begin an object lifetime or make unrelated storage
safe to access.

Structures can define matching conversion constructors with reserved parameter
names such as `@OTHER`, `@EXPLICIT`, `@CHECKED`, and `@REINTERPRET`.

Polymorphic instance pointers use `AS DYNAMIC` for checked downcasts and
cross-casts. See [Inheritance](inheritance.md); that operation is not
implemented by the JVM backend.

`AS UNCHECKED_STATIC_DOWNCAST` recovers a known exact complete struct
type from a base pointer or reference. The conversion supports nonpolymorphic
structs and requires an unambiguous source base. The exact complete type of the
object is a precondition. The conversion performs no runtime check and cannot
be overloaded. See
[the unchecked downcast rules](../reference/inheritance.md#unchecked-static-downcasts).

## Reference

See the [Conversions Reference](../reference/conversions.md) for every mode,
narrowing rules, pointer constraints, and user-defined conversion selection.
