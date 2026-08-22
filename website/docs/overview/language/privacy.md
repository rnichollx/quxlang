# Overview of Privacy

`PRIVATE(...)` limits which source contexts may name a declaration. It is a
compile-time access rule; it does not change object layout or the external ABI.

## Restrict one declaration

```quxlang
PRIVATE(MODULE) ::initialize FUNCTION()
{
}
```

`PRIVATE(MODULE)` allows use from the current module. A named scope can be used
instead when a particular namespace or type owns the API.

## Restrict structure members

```quxlang
::vault STRUCT
{
  PRIVATE(CLASS)
  {
    .value VAR I32;
  }

  .read FUNCTION() CONST: I32
  {
    RETURN .value;
  }
}
```

`PRIVATE(CLASS)` grants access from the nearest enclosing class-like
declaration and its nested contexts. A privacy block applies its scope list to
each declaration directly inside the block.

## Grant a named scope access

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

When several scopes appear in `PRIVATE(...)`, access through any one is enough.
Privacy is checked during qualified lookup and overload selection; it does not
search for a different public declaration after selecting an inaccessible one.

## Reference

See the [Privacy Reference](../../reference/privacy.md) for all scope entries,
block restrictions, qualified-name behavior, shadowing, overloads, reopening,
and public signatures that mention private types.
