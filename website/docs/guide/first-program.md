# First Program

A Quxlang source file begins with a language declaration. Imports follow it,
then module-level declarations begin with `::name`.

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;

::add_numbers FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

::main FUNCTION(): I32
{
  VAR total I32 := add_numbers(% [2, 3]);
  ASSERT(total == 5);
  RETURN 0;
}
```

This example establishes several rules used throughout the language:

- Keywords are uppercase and user identifiers are lowercase.
- `::add_numbers` and `::main` are declarations in the current module.
- `%left` and `%right` are positional parameters.
- `% [2, 3]` is an explicit positional argument group. `add_numbers(2, 3)` is
  not valid Quxlang call syntax.
- `: I32` declares the return type.
- `VAR total I32 := ...;` declares and initializes a mutable local object.
- `:=` initializes or assigns; `==` compares.

The compiler operates on the enclosing source bundle. Given a bundle directory,
a direct compiler invocation has this shape:

```console
qxc ./example-bundle ./out
```

The current public invocation compiles the targets configured in
`quxbuild.yaml`. Each target's output definition decides whether its result is
an executable, unit-test suite, or another output form.

Next read [Source bundles](source-bundles.md),
[Source files and imports](../language/source-files-and-imports.md), and
[Call arguments](../language/call-arguments.md).
