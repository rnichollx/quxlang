# Overview of Declaration Documentation

A `DOC <$ ... $>` block attaches reader-facing documentation to the declaration
that follows it:

```quxlang
::double INCLUDE_IF(TRUE) DOC <$
  Returns twice the supplied value.
  $>
  FUNCTION(@ARG:value I32): I32
{
  RETURN value * 2;
}
```

For an ordinary named declaration, modifiers occur in this order:

1. the declaration name;
2. optional `INCLUDE_IF(...)`;
3. optional `DOC <$ ... $>`;
4. the declaration kind and definition.

Member documentation follows the same placement:

```quxlang
::counter STRUCT
{
  .read DOC <$ Returns the current counter value. $>
    FUNCTION() CONST: I32
  {
    RETURN .value;
  }

  .value VAR I32;
}
```

Documentation text should describe the public contract. Compiler query names,
AST shapes, and lowering mechanics are not part of a source-facing declaration
contract.

See [Availability and targets](availability-and-targets.md).

## Complete technical rules

See the [Declaration Documentation Reference](../../reference/declaration-documentation.md) for the complete
language rules, constraints, and technical edge cases.
