# Lexical Structure

Quxlang makes keywords and user-defined names visually distinct. Keywords use
uppercase spelling; source identifiers use lowercase spelling.

## Identifiers and keywords

A user identifier:

- begins with `a` through `z`;
- continues with lowercase letters, decimal digits, or `_`; and
- does not end with `_`.

Valid examples include `value`, `value2`, and `source_module`. `Value`,
`SOURCE_MODULE`, `_value`, and `value_` are not user identifiers.

Language keywords and reserved compiler names are uppercase, such as
`FUNCTION`, `STATIC_IF`, and `STRING_CONSTANT`. This division lets Quxlang add
new keyword spellings without colliding with ordinary source names.

## Whitespace and comments

Spaces, tabs, and line breaks separate tokens. A line comment begins with `//`
and continues to the end of the line:

```quxlang
VAR count I32 := 4; // Number of items to process.
```

Block-comment syntax is not part of the current source grammar.

## Numeric literals

Numeric literals use decimal digits with at most one decimal point:

```quxlang
VAR integer I32 := 42;
VAR fraction F64 := 12.5;
```

The literal itself remains an exact compile-time literal until context selects
a concrete numeric type. A leading minus is expressed as subtraction, commonly
from zero:

```quxlang
VAR negative I32 := 0 - 7;
```

Current source literals do not use C-style hexadecimal, octal, binary, digit
separator, or exponent notation.

## String and character literals

Strings use double quotes and characters use single quotes:

```quxlang
VAR message STRING_CONSTANT := "line one\nline two";
VAR newline BYTE := '\n';
```

String escapes are `\n`, `\t`, `\r`, `\0`, `\\`, and `\"`. Character
literals support the same byte escapes, with `\'` in place of the escaped
double quote. A character literal represents one `BYTE`, not a multi-byte
Unicode scalar.

See [Primitive types and literals](primitive-types-and-literals.md) and
[Source files and imports](source-files-and-imports.md).
