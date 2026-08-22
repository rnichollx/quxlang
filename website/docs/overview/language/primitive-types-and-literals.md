# Overview of Primitive Types

Quxlang uses explicit primitive names so a declaration communicates its value
category and, for most numbers, its exact width.

## Choose a numeric type

```quxlang
VAR condition BOOL := TRUE;
VAR octet BYTE := 255;
VAR signed_value I32 := 0 - 7;
VAR unsigned_value U24 := 16777215;
VAR real_value F32 := 1.5;
VAR element_count SZ := 4;
```

`I<N>` and `U<N>` are signed and unsigned integers with `N` bits. `F32` and
`F64` are the common floating-point types. `SZ` and `UINTPTR` are
pointer-sized unsigned forms. `BYTE` is intentionally distinct from `U8`.

## Use nonnumeric built-ins

```quxlang
VAR raw_address ADDRESS;
VAR ordering ORDER := ORDER::EQUAL;
VAR text STRING_CONSTANT := "hello";
```

Other built-ins include `VOID`, `TYPE_INDEX`, and `NULL_TYPE`.
`STRING_CONSTANT`, `CSTRING_CONSTANT`, `DATA_CONSTANT`, and
`NUMERIC_CONSTANT` hold read-only compile-time data rather than mutable runtime
containers.

## Let context type a literal

```quxlang
VAR small I8 := 42;
VAR large I64 := 42;
VAR newline BYTE := '\n';
VAR absent CONST->I32 := NULL;
```

Numeric and string literals keep their literal identity until the surrounding
declaration, call, or conversion chooses a destination type. Template patterns
can also match one exact literal or bind any numeric or string literal.

Some layouts are target-dependent. Guard a nonstandard floating layout or
layout-specific operation with the relevant target predicate.

## Complete technical rules

See the [Primitive Types and Literals Reference](../../reference/primitive-types-and-literals.md)
for every built-in category, arbitrary-width spellings, literal-pattern types,
constant spans, and target restrictions.
