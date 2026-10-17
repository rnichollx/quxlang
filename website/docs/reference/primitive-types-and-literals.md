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

The bit count of an integer type specifies its value width. The size and alignment
of all integer types, including `I8`, `I32`, `U64`, and nonstandard-width
types such as `U24`, are unspecified by the language and are not guaranteed
across targets or compiler versions. Object storage can include padding. `SIZEOF` and `ALIGNOF` report the
object size and alignment selected for the current target. An object's cell
size, which determines the spacing between array elements, can exceed its
object size to satisfy alignment requirements. For example, an object with
size 3 and alignment 2 can have cell size 4. Equality between object size and
cell size is not guaranteed. The current LLVM backend follows LLVM's integer allocation sizes
and ABI alignments and verifies agreement during lowering.

On most platforms, integer types with value widths of 8, 16, 32, or 64 bits
occupy 1, 2, 4, or 8 bytes, respectively. Their alignment often equals their
size. These are typical platform behaviors rather than language guarantees.

Nonstandard floating layouts are target-dependent. The current LLVM backend,
for example, supports this guarded declaration:

```quxlang
STATIC_IF(BACKEND_LLVM)
{
  VAR custom_float F16E5 := 1.5;
}
```

## Computed floating-point types

`FLOAT(total_bits, exponent_bits)` constructs the same type as
`F<total_bits>E<exponent_bits>`. Both arguments are compile-time expressions
representable as unsigned 64-bit integers. The total width includes the sign
bit, exponent field, and stored fraction field. The total width must be at
least three, and the exponent width must be positive and smaller than
`total_bits - 1`. Invalid widths produce a semantic compilation error.

```quxlang
::computed_float_type_test STATIC_TEST
{
  VAR value FLOAT(64, 11) := 1.5;
  TEST_ASSERT(SAME_TYPES(FLOAT(64, 11), F64));
  TEST_ASSERT(SAME_TYPES(FLOAT(16 + 16, 8), F32));
}
```

Type construction does not imply native target support for that format. The
LLVM backend currently represents `F16E5`, `F16E8`, `F32E8`, `F64E11`, and
`F128E15`. Compile-time evaluation supports additional binary formats.

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
