# Names, Namespaces, and Scopes

The prefix on a declaration makes its owning scope visible:

```quxlang
::global_count VAR I32 := 0;

::metrics NAMESPACE
{
  ::sample_count STATIC U64 := 4;
}

::counter STRUCT
{
  .value VAR I32;

  .increment FUNCTION()
  {
    .value++;
  }

  ::initial_value STATIC I32 := 0;
}
```

- `::name` declares a module, namespace, or type-level name.
- `.name` declares an instance member.
- `namespace::name` and `module::name` select nested declarations.
- `.name` inside a member function selects a member of `THIS`.
- A nested `::name` function or object belongs to the type but does not receive
  an instance.

Declarations are order-independent: a declaration may name another declaration
from its source bundle without requiring textual forward declarations.

Keywords are uppercase. User identifiers are lowercase and may contain digits
and underscores; mixed-case identifiers are not part of the current source
convention enforced by the language.

See [Structs and members](structs-and-members.md) and [Privacy](privacy.md).

