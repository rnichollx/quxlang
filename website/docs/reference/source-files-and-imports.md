# Source Files and Imports

Every source file begins with a language header, followed by declarations:

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;
::filesystem IMPORT foo_filesystem_utilities;
```

The header is required in every file, after optional whitespace and comments.
`QUXLANG`, `EN`, version `0.0`, and the terminating semicolon are required.
The source bundle assigns each file to a logical module.

## Import declarations

```text
IMPORT module_name [AS local_name];
::local_name IMPORT module_name;
IMPORT_IF(condition) module_name [AS local_name];
::local_name IMPORT_IF(condition) module_name;
```

Module and alias names use lowercase identifiers. An explicitly named import
cannot also use `AS`. Imports are declarations and may appear among other
declarations or inside namespaces:

```quxlang
::io NAMESPACE
{
  ::fs IMPORT foo_filesystem_utilities;
}

VAR path io::fs::path;
```

An import creates an alias in its enclosing declaration scope, visible across
source files belonging to that logical module. Repeated imports of the same
module under the same name are accepted. Conflicting declarations are errors.
Imports are module-private; importing a module does not publicly re-export it.
Use an accessible `ALIAS` declaration to expose an imported owner:

```quxlang
::implementation IMPORT foo_filesystem_utilities;
::filesystem ALIAS implementation;
```

## Conditional imports

`IMPORT_IF` evaluates a compile-time condition and includes the import only
when it is true:

```quxlang
IMPORT_IF(OS_LINUX) linux_support;
IMPORT_IF(OS_WINDOWS) windows_support;
```

An explicitly named import may instead use `INCLUDE_IF` before `IMPORT`.
Combining `INCLUDE_IF` and `IMPORT_IF` on one declaration is invalid. Imports
always have `PRIVATE(MODULE)` access; a conflicting privacy scope is invalid.

An inactive import does not require its module to be configured. Conditions
follow the declaration availability rules described in
[Target Availability](availability-and-targets.md).

## Logical module resolution

`IMPORT foolib;` requests the logical module `foolib` from the active target.
The target's module map selects its source. The import names a module rather
than a physical source file or directory. Qualified lookup uses `::`:

```quxlang
VAR answer I32 := foolib::imported_function();
```

Imported declarations retain their privacy and availability restrictions.
Declarations from all active source files contribute to their logical module;
lookup does not depend on source-file order.

See [Source Bundles and Targets](../overview/source-bundles.md),
[Namespaces](namespaces.md), and [Privacy](privacy.md).
