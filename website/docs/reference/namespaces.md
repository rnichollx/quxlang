# Namespaces

A namespace is a named declaration scope. It groups related declarations under
one qualified name and has no runtime object, storage, or lifetime of its own.

## Declaration grammar

A namespace declaration has this form:

```text
::name NAMESPACE
{
  declarations
}
```

The leading `::` is part of the declaration name syntax. No semicolon follows
the closing brace.

```quxlang
::geometry NAMESPACE
{
  ::origin STATIC I32 := 0;

  ::distance FUNCTION(@left I32, @right I32): I32
  {
    RETURN right - left;
  }
}
```

Every declaration directly inside the namespace uses its own declaration
prefix. `::name` adds a namespace-level declaration; `.name` is not valid as a
namespace member because dot-prefixed declarations are instance members of a
type.

An empty namespace body is valid.

## Qualification

Use `::` between a namespace name and a declaration owned by it:

```quxlang
VAR span I32 := geometry::distance(
  @left geometry::origin,
  @right 8
);
```

Qualification can continue through multiple namespace, type, or enum owners:

```quxlang
VAR flags syscall::linux::open_flags :=
  syscall::linux::open_flags::rdonly;
```

The same separator is used for several kinds of symbolic ownership. The kind of
the left-hand name determines what the next component can select:

- a module or namespace selects a nested declaration;
- a type selects a type-owned declaration, nested type, or enum/flagset value;
- a value is not qualified with `::`; its instance members use `.`.

Inside a namespace, an unqualified name can resolve to another declaration in
that namespace or through an enclosing context. Explicit qualification is
useful when two visible declarations have the same short name or when the owner
is important to the reader.

## Nested namespaces

Namespaces may contain namespaces:

```quxlang
::network NAMESPACE
{
  ::http NAMESPACE
  {
    ::default_port STATIC U16 := 80;
  }
}

::port FUNCTION(): U16
{
  RETURN network::http::default_port;
}
```

The nested declaration `::http` belongs to `network`; it does not create an
unrelated top-level namespace.

## Reopening a namespace

The same namespace may be declared more than once. Its active declarations are
combined into one namespace:

```quxlang
::protocol NAMESPACE
{
  ::version STATIC U32 := 1;
}

::protocol NAMESPACE
{
  ::is_supported FUNCTION(@value U32): BOOL
  {
    RETURN value == version;
  }
}
```

Reopening is useful for organizing one namespace across source files. Source
order does not determine visibility: declarations in the active source bundle
are resolved symbolically, so the second body can refer to `version` even when
the files are collected in a different order.

Each declaration still has its own availability and privacy. A private
declaration does not become public merely because another public namespace body
reopens the same namespace. See [Privacy](privacy.md) and
[Target Availability](availability-and-targets.md).

## Modules and imports

A module is also a symbolic owner, but it is selected by the build and declared
in the source-file header rather than with `NAMESPACE`. After importing a
module, its public declarations can be qualified with the imported module name:

```quxlang
IMPORT std;

VAR message std::string := "ready";
```

`std::string` means the `string` declaration owned by the imported `std`
module. An import does not copy that declaration into the current namespace.
See [Source Files and Imports](source-files-and-imports.md) for module headers,
import syntax, and import visibility.

## Names and reserved subentities

Ordinary identifiers begin with `a` through `z`. Later characters may be
lowercase letters, digits, or underscores. An identifier cannot end in an
underscore. Consequently, `cache2` and `http_status` are identifiers, while
`Cache`, `2cache`, and `cache_` are not.

Uppercase spellings are keywords or reserved built-in subentity names. Some
reserved names, including `CONSTRUCTOR`, `DESTRUCTOR`, and operator names, are
valid only in the declaration contexts that define their meaning. They are not
ordinary user-chosen namespace identifiers.

## Namespaces versus type members

The two declaration prefixes encode different ownership:

```quxlang
::counter STRUCT
{
  .value VAR I32;
  .increment FUNCTION() { .value++; }
  ::initial STATIC I32 := 0;
}
```

`.value` and `.increment` require a `counter` instance and use `.` at an access
site. `::initial` belongs to the type itself and is selected as
`counter::initial`. A namespace can own the latter kind of declaration but can
never own an instance member because it has no instance.

See [Structures](structs-and-members.md) for `THIS`, member lookup, and
type-owned declarations.
