# Overview of Conversions

Conversions change a value's type. Quxlang asks you to state risky conversion
semantics explicitly so truncation, runtime validation, assumptions, and
representation changes remain visible in source.

## Choose the intended conversion

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

## Reinterpret an allowed representation

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

## Complete technical rules

See the [Conversions Reference](../../reference/conversions.md) for every mode,
narrowing rules, pointer constraints, and user-defined conversion selection.
