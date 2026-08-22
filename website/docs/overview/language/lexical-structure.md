# Overview of Lexical Structure

Quxlang source makes language vocabulary visually distinct: keywords are
uppercase, while programmer-defined identifiers are lowercase.

## Name things

An identifier begins with `a` through `z`, continues with lowercase letters,
digits, or `_`, and does not end in `_`:

```quxlang
VAR item_count I32 := 4;
::source_module NAMESPACE
{
}
```

Names such as `Value`, `_value`, and `value_` are not user identifiers.
Uppercase words such as `VAR`, `FUNCTION`, and `STATIC_IF` belong to the
language and compiler.

## Write comments and numbers

```quxlang
VAR count I32 := 42; // A line comment continues to the line end.
VAR fraction F64 := 12.5;
VAR negative I32 := 0 - 7;
```

Numeric tokens use decimal digits with at most one decimal point. The minus in
a negative value is an operator, not part of the numeric token. Current source
does not use C-style hexadecimal, binary, exponent, or digit-separator syntax.

## Write strings and bytes

```quxlang
VAR message STRING_CONSTANT := "line one\nline two";
VAR newline BYTE := '\n';
```

Double quotes form strings and single quotes form one-byte character literals.
Common escapes include newline, tab, carriage return, zero, and escaped quote
or backslash characters.

## Reference

See the [Lexical Structure Reference](../../reference/lexical-structure.md) for
the exact identifier grammar, whitespace and comment rules, numeric forms, and
all supported literal escapes.
