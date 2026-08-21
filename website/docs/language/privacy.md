# Privacy

Declarations are public unless prefixed with `PRIVATE(...)`. Privacy is a
compile-time name-access rule; it does not change layout, linkage, symbol
mangling, or ABI.

## Single declarations and blocks

Privacy appears before a declaration name:

```quxlang
PRIVATE(MODULE) ::initialize FUNCTION()
{
}
```

A block applies one scope list to each declaration directly inside it:

```quxlang
::vault STRUCT
{
  PRIVATE(CLASS)
  {
    .value VAR I32;
  }

  .read FUNCTION(): I32
  {
    RETURN .value;
  }
}
```

The privacy of a class or namespace declaration is not automatically inherited
by its members. Annotate nested declarations separately when they need their
own restriction. A `PRIVATE` block cannot be nested directly inside another
`PRIVATE` block.

## Scope entries

- `PRIVATE(CLASS)` grants access to the nearest enclosing `STRUCT`, `UNION`,
  `VARIANT`, or `IMPLEMENTATION`, including nested contexts.
- `PRIVATE(MODULE)` grants access to the declaration's current module.
- A type-symbol entry grants access to that named context and its nested
  contexts.

```quxlang
PRIVATE(trusted_scope) ::shared_secret FUNCTION(): I32
{
  RETURN 11;
}

::trusted_scope NAMESPACE
{
  ::read_secret FUNCTION(): I32
  {
    RETURN shared_secret();
  }
}
```

Several entries are alternatives; access through any listed scope is enough. A
declaration is always accessible from its own nested contexts.

`RUNTIME_MODULE` is the source-visible absolute reference to the runtime module
and may be used as a named privacy scope. `MODULE` is valid by itself only as
the current-module entry in `PRIVATE(MODULE)`.

## Qualified lookup

Every lexical prefix in a qualified name must be accessible. Access to
`a::b::c` fails when `a::b` is private even if `c` is public. A private
declaration still shadows an outer declaration with the same name; inaccessible
lookup does not resume in search of a public fallback.

When overload resolution selects an inaccessible declaration, compilation
fails. Resolution is not repeated to find a less-specific public candidate.

Reopened non-overload declarations must use equivalent privacy scopes. Mixing
public and private reopenings, or reopening with a different scope list, is an
error.

Privacy does not prevent a public declaration from exposing a private type in
its signature.
