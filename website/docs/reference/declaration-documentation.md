# Declaration Documentation

`DOC <$ ... $>` attaches reader-facing text to the declaration that
follows it. The documentation becomes declaration metadata; it does not
execute and does not change overload or availability semantics.

## Placement

For an ordinary named declaration, the components appear in this order:

1. optional `DOC <$ ... $>`;
2. optional `PRIVATE(...)`;
3. the `.name` or `::name` declaration name;
4. optional `INCLUDE_IF(...)`;
5. the declaration kind and definition.

```quxlang
DOC <$
  Returns twice the supplied value.
  $>
::double INCLUDE_IF(TRUE) FUNCTION(@ARG:value I32): I32
{
  RETURN value * 2;
}
```

Member and type-owned declarations use the same position:

```quxlang
::counter STRUCT
{
  DOC <$ Returns the current counter value. $>
  .read FUNCTION() CONST: I32
  {
    RETURN .value;
  }

  DOC <$ Current mutable count. $>
  .value VAR I32;
}
```

The `DOC` block precedes the declaration name and any explicit privacy modifier.
Only one `DOC` block is allowed for one declaration. A block after the declaration
name, a block without a following declaration, and a block preceding a scoped
`PRIVATE` block are syntax errors. Declarations inside a scoped `PRIVATE` block
may each carry their own prefix documentation.

The same placement applies to named and anonymous base declarations and import
declarations. Documentation belongs to the following declaration alone; nested
declarations and subsequent declarations receive their own documentation.

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
DOC <$
  Parses one value.

  Failure is reported to the caller.
  $>
::parse FUNCTION()
{
}
```

This permits the source block to follow its surrounding indentation without
embedding that structural indentation in the stored documentation. Relative
indentation beyond the closing delimiter's indentation is preserved.

Whitespace and comments after `$>` are skipped before parsing the declaration.

## Availability and privacy

`DOC` attaches to the declaration before target filtering and privacy checks.
It does not make an unavailable declaration active and does not make a private
declaration public. The ordering with `INCLUDE_IF` is syntactic: when both are
present, `DOC` precedes the declaration name and `INCLUDE_IF(...)` follows the name.

See [Target Availability](availability-and-targets.md) and
[Privacy](privacy.md).

## Content contract

Documentation should describe the programming interface: purpose, parameters,
return value, ownership, mutation, lifetime, failure behavior, and important
constraints. Compiler query names, AST node layouts, and lowering mechanics are
not part of a source-facing declaration contract unless the declaration itself
is an explicitly compiler-facing interface.
