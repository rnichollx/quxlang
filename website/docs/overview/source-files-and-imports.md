# Overview of Source Files and Imports

Every Quxlang source file starts with a language declaration:

```quxlang
LANGUAGE QUXLANG EN 0.0;
```

`QUXLANG` identifies the language, `EN` selects its keyword language, and `0.0`
is the language version accepted by the current source fixtures.

Imports name logical modules from the active target configuration:

```quxlang
IMPORT foolib;
IMPORT foo_filesystem_utilities AS fs;
```

The first import exposes the logical module as `foolib`. `AS` changes only its
local source name. Imported declarations are qualified with `::`:

```quxlang
VAR answer I32 := foolib::imported_function();
VAR path fs::path;
```

A source file does not declare which source directory supplies `foolib`; the
target's module map makes that decision. This lets different targets select
different versions or platform implementations without changing import sites.

Comments use `//` through the end of a line:

```quxlang
// A source comment.
VAR count I32 := 4;
```

See [Source bundles and targets](../guide/source-bundles.md),
[Lexical structure](lexical-structure.md),
[Names and scopes](namespaces.md), and
[Declaration documentation](declaration-documentation.md).

## Reference

See the [Source Files and Imports Reference](../reference/source-files-and-imports.md) for the complete
language rules, constraints, and technical edge cases.
