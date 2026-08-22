# Declaration Documentation

`DOC <$ ... $>` attaches reader-facing text to the named declaration that
contains it. The documentation becomes declaration metadata; it does not
execute and does not change overload or availability semantics.

## Placement

For an ordinary named declaration, the components appear in this order:

1. the `.name` or `::name` declaration name;
2. optional `INCLUDE_IF(...)`;
3. optional `DOC <$ ... $>`;
4. the declaration kind and definition.

```quxlang
::double INCLUDE_IF(TRUE) DOC <$
  Returns twice the supplied value.
  $>
  FUNCTION(@ARG:value I32): I32
{
  RETURN value * 2;
}
```

Member and type-owned declarations use the same position:

```quxlang
::counter STRUCT
{
  .read DOC <$ Returns the current counter value. $>
    FUNCTION() CONST: I32
  {
    RETURN .value;
  }

  .value DOC <$ Current mutable count. $> VAR I32;
}
```

Documentation before the declaration name is not attached by this grammar.
Only one `DOC` block is allowed for one declaration.

## Delimiters and text

The opening delimiter is `<$` and the closing delimiter is `$>`:

```quxlang
DOC <$ One-line documentation. $>
```

All text between the delimiters is captured. A dollar sign that is not followed
by `>` remains ordinary documentation text. Reaching the end of the source file
without `$>` is a syntax error.

For a multiline block, the indentation of the line containing the closing
delimiter is removed from the beginning of each line that has that indentation:

```quxlang
::parse DOC <$
  Parses one value.

  Failure is reported to the caller.
  $>
  FUNCTION()
{
}
```

This permits the source block to follow its surrounding indentation without
embedding that structural indentation in the stored documentation. Relative
indentation beyond the closing delimiter's indentation is preserved.

Whitespace and comments after `$>` are skipped before parsing the declaration
kind.

## Availability and privacy

`DOC` attaches to the declaration before target filtering and privacy checks.
It does not make an unavailable declaration active and does not make a private
declaration public. The ordering with `INCLUDE_IF` is syntactic: when both are
present, `INCLUDE_IF(...)` precedes `DOC`.

See [Target Availability](availability-and-targets.md) and
[Privacy](privacy.md).

## Content contract

Documentation should describe the programming interface: purpose, parameters,
return value, ownership, mutation, lifetime, failure behavior, and important
constraints. Compiler query names, AST node layouts, and lowering mechanics are
not part of a source-facing declaration contract unless the declaration itself
is an explicitly compiler-facing interface.
