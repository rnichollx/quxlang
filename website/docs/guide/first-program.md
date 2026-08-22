# First Program

A Quxlang source file begins with a language declaration. Imports follow it,
then module-level declarations begin with `::name`.

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;

::clamp FUNCTION(@value I32, @minimum I32, @maximum I32): I32
{
  IF (value < minimum)
  {
    RETURN minimum;
  }
  IF (value > maximum)
  {
    RETURN maximum;
  }
  RETURN value;
}

::main FUNCTION(): I32
{
  VAR volume I32 := clamp(@value 120, @minimum 0, @maximum 100);
  ASSERT(volume == 100);
  RETURN 0;
}
```

This example establishes several rules used throughout the language:

- Keywords are uppercase and user identifiers are lowercase.
- `::clamp` and `::main` are declarations in the current module.
- `@value`, `@minimum`, and `@maximum` are named parameters. Their names make
  the roles of three same-typed arguments explicit at the call site.
- Quxlang also supports explicit positional argument groups for APIs where
  position is meaningful; the [Call arguments](../reference/call-arguments.md)
  page introduces those after named arguments.
- `: I32` declares the return type.
- `VAR volume I32 := ...;` declares and initializes a mutable local object.
- `:=` initializes or assigns; `==` compares.

The compiler operates on the enclosing source bundle. Given a bundle directory,
a direct compiler invocation has this shape:

```console
qxc ./example-bundle ./out
```

The current public invocation compiles the targets configured in
`qxcbuild.yml`. Each target's output definition decides whether its result is
an executable, unit-test suite, or another output form.

Next read [Source bundles](source-bundles.md),
[Source files and imports](../reference/source-files-and-imports.md), and
[Call arguments](../reference/call-arguments.md).
