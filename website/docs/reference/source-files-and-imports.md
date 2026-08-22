# Source Files and Imports

Every Quxlang source file has one language header, followed by zero or more
imports, followed by its declarations. A file does not contain a source-level
module declaration; the source bundle assigns the file to a logical module.

## Language header

After optional leading whitespace and comments, the file must begin with:

```quxlang
LANGUAGE QUXLANG EN 0.0;
```

All four components are required:

- `LANGUAGE` introduces the header;
- `QUXLANG` identifies Quxlang;
- `EN` selects the English keyword vocabulary;
- `0.0` is the currently accepted source-language version.

The version must be exactly `0.0`, and the semicolon is required. The header is
per file, not once per source directory or module.

## Import grammar

Imports appear immediately after the language header and before any ordinary
declaration:

```text
IMPORT module_name [AS local_name];
```

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;
IMPORT foo_filesystem_utilities AS fs;
```

Both names use the lowercase identifier grammar. Without `AS`, the imported
module's local name is its configured logical module name. `AS` changes only
the name used in this file:

```quxlang
VAR text std::string := "ready";
VAR path fs::path;
```

Imports are file-local. Another source file in the same logical module must
write its own `IMPORT` declarations for the module names it uses.

## Logical module resolution

`IMPORT foolib;` requests the logical module named `foolib` from the active
source-bundle target. It does not name a directory, repository, or physical
file. The target's module map selects the source and version that provide that
logical module.

This indirection permits different targets to bind the same import name to
different compatible module sources without changing Quxlang source code.
Failure to provide the imported logical module is a build-configuration error.

An import does not textually include another file and does not impose source
declaration order. It makes the imported module owner available for qualified
symbol lookup. Only declarations accessible under their privacy and target
availability rules can be selected.

## Qualified imported names

Use `::` between the import's local name and a declaration owned by that module:

```quxlang
VAR answer I32 := foolib::imported_function();
VAR output fs::path;
```

Further qualification can select namespaces or type-owned declarations:

```quxlang
VAR mode syscall::linux::file_mode := syscall::linux::file_mode::rusr;
```

The import name itself is a module owner, not a runtime value or namespace
declaration. See [Namespaces](namespaces.md) for qualification rules.

## Declaration portion

Once the parser encounters the first non-`IMPORT` declaration, the remainder of
the file is parsed as declarations. An `IMPORT` cannot be introduced later
between declarations.

Declarations from all active files assigned to the logical module contribute to
the module's symbolic contents. Their semantic visibility is not based on file
order, so forward declarations are not required merely because a definition is
located in another source file.

See [Source Bundles and Targets](../guide/source-bundles.md) for the physical
directory and target mapping, [Lexical Structure](lexical-structure.md) for
comments and tokens, and [Privacy](privacy.md) for cross-module access.
