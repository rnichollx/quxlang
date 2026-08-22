# Overview of Namespaces

Namespaces group related declarations and keep their short names from
colliding with declarations in other parts of a program.

## Declaring a namespace

Put the namespace name before `NAMESPACE`, then place its declarations in the
body:

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

The `::` before `geometry`, `origin`, and `distance` marks each as a named
declaration in its current owner. It is not punctuation that can be omitted.

## Using names from a namespace

Write `namespace::name` to select a declaration:

```quxlang
::measure FUNCTION(): I32
{
  RETURN geometry::distance(
    @left geometry::origin,
    @right 12
  );
}
```

Inside the namespace, sibling declarations can usually use one another by
their short names. Outside it, qualification makes the owner explicit.

## Nested namespaces

Namespaces can be nested to describe a larger hierarchy:

```quxlang
::network NAMESPACE
{
  ::http NAMESPACE
  {
    ::default_port STATIC U16 := 80;
  }
}

::configured_port FUNCTION(): U16
{
  RETURN network::http::default_port;
}
```

Use enough nesting to express a real ownership relationship. A namespace has
no runtime value and cannot be constructed like a structure.

## Extending a namespace

The same namespace can be reopened:

```quxlang
::geometry NAMESPACE
{
  ::minimum STATIC I32 := 0;
}

::geometry NAMESPACE
{
  ::maximum STATIC I32 := 100;
}
```

Both declarations belong to `geometry`. Reopening lets several source files
contribute related declarations without placing them in one large file.

## Namespaces, modules, and structures

An imported module uses the same `::` qualification separator:

```quxlang
IMPORT std;

VAR message std::string := "ready";
```

Here `std` is a module, not a namespace. Modules are selected by source headers
and the build; namespaces are declared within a module.

Structures use `.` for instance members and `::` for declarations owned by the
type:

```quxlang
VAR value counter;
value.increment();
VAR initial I32 := counter::initial;
```

For reopening, identifier, qualification, privacy, and lookup rules, see the
[Namespaces Reference](../../reference/namespaces.md).
