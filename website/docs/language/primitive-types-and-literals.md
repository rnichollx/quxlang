# Primitive Types and Literals

Quxlang uses explicit-width primitive names:

```quxlang
VAR condition BOOL := TRUE;
VAR octet BYTE := 255;
VAR signed_value I32 := 0 - 7;
VAR unsigned_value U128 := 340282366920938463463374607431768211455;
VAR unusual_width U24 := 16777215;
VAR real_value F32 := 1.5;
VAR element_count SZ := 4;
VAR raw_address ADDRESS;
VAR ordering ORDER := ORDER::EQUAL;
VAR text STRING_CONSTANT := "hello";
VAR number NUMERIC_CONSTANT := 42;
```

## Numeric types

- `I<N>` and `U<N>` are signed and unsigned integers with `N` bits.
- `F32` and `F64` use their standard exponent widths.
- Other floating layouts use `F<N>E<M>`, where `M` is the exponent width.
- `SZ` and `UINTPTR` are pointer-sized unsigned integer forms.
- `BYTE` is distinct from `U8`.

Nonstandard floating layouts are target-dependent. The current LLVM backend,
for example, supports this guarded declaration:

```quxlang
STATIC_IF(BACKEND_LLVM)
{
  VAR custom_float F16E5 := 1.5;
}
```

## Other built-in types

`BOOL`, `ADDRESS`, `TYPE_INDEX`, `ORDER`, `VOID`, and `NULL_TYPE` are distinct
built-in types. `STRING_CONSTANT`, `CSTRING_CONSTANT`, `DATA_CONSTANT`, and
`NUMERIC_CONSTANT` are compile-time constant categories rather than ordinary
runtime string or number containers.

## Literal values

Source literals include decimal integers, floating-point values, characters,
strings, `TRUE`, `FALSE`, and `NULL`. `UNSPECIFIED` requests unspecified initial
state where the target type permits it.

Numeric and string literals retain their literal identity until context chooses
a destination. The type-symbol forms used for exact and deduced literal
matching are:

| Form | Meaning |
| --- | --- |
| `NUMERIC_LITERAL_TYPE("42")` | The exact numeric literal `42` |
| `NUMERIC_LITERAL_ANY(name)` | Any numeric literal, bound as `name` |
| `STRING_LITERAL_TYPE("text")` | The exact string literal `"text"` |
| `STRING_LITERAL_ANY(name)` | Any string literal, bound as `name` |

The `*_ANY(name)` forms are template-matching patterns rather than runtime
container types.

## Read-only constant categories

`STRING_CONSTANT` and `NUMERIC_CONSTANT` preserve computed compile-time data in
a form that may be embedded in an output:

```quxlang
::message STATIC STRING_CONSTANT := "hello";
::number STATIC NUMERIC_CONSTANT := 42;

::constant_access STATIC_TEST
{
  ASSERT(message.END() - message.BEGIN() == 5);
}
```

`STRING_CONSTANT`, `CSTRING_CONSTANT`, `DATA_CONSTANT`, and
`NUMERIC_CONSTANT` are read-only byte-span categories with built-in `BEGIN()`
and `END()` access. Only the conversions supplied for a category are valid; in
particular, these names do not denote mutable runtime string or byte-vector
containers.

The exact token forms and escape sequences are documented on
[Lexical structure](lexical-structure.md). Numeric conversion rules are covered
on [Conversions](conversions.md).
