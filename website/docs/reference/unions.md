# Unions

A Quxlang union stores one active, named option. Each option has its own payload
type, and the option name is part of construction, observation, and `MATCH`
syntax.

## Declaration forms

`INLINE_UNION` stores its active payload inside the union object:

```quxlang
::status INLINE_UNION
{
  .ok OPTION DEFAULT VOID;
  .error OPTION I32;
}
```

`UNION` uses boxed payload storage:

```quxlang
::node UNION
{
  .end OPTION DEFAULT VOID;
  .next OPTION node;
}
```

Boxed storage permits a directly recursive option such as `node.next` because
the payload is not embedded inline in the containing object. Use
`INLINE_UNION` when every alternative has a finite inline layout and the object
should contain its payload directly.

A union must declare at least one option. Option names must be unique. The
payload type is resolved in the union's declaration context.

## Options

An option declaration has this form:

```text
.name OPTION [DEFAULT] Type;
```

The dot prefix identifies a named option of the union. An option whose type is
`VOID` carries no payload. At most one option may have `DEFAULT`.

The union body may also contain associated declarations such as functions,
tests, or type-owned constants:

```quxlang
::result INLINE_UNION
{
  .value OPTION DEFAULT I32;
  .failure OPTION U32;

  ::is_zero FUNCTION(@ARG:value CONST& result): BOOL
  {
    MATCH value AS payload
    {
      CASE value { RETURN payload == 0; }
      CASE failure { RETURN FALSE; }
    }
  }
}
```

Associated declarations use the ordinary rules for declarations owned by a
type. They are selected with `result::name` unless declared as instance
members.

## Construction

The generated default constructor selects the `DEFAULT` option and
default-constructs its payload:

```quxlang
VAR success status;
ASSERT(success IS ok);
```

If no option is marked `DEFAULT`, ordinary default construction is available
only when `VALUELESS_DEFAULT` is specified or a user-defined constructor
provides another path.

Each option receives a generated named constructor argument. Supplying that
name selects the option:

```quxlang
VAR failure status :(@error 5);
ASSERT(failure IS error);
```

The argument initializes the option's payload. Constructor overload resolution
still applies, so a user-declared constructor may be selected ahead of a
generated baseline when it is a better match.

Copy and move construction from another object of the same union use the
active option and the corresponding payload operation. A moved-from union may
become valueless unless `NEVER_VALUELESS` requires it to be reset to the default
option.

## Testing the active option

`value IS option_name` produces `BOOL`:

```quxlang
IF (failure IS error)
{
  handle_error();
}
```

The subject must be a union, and the option must exist in that union. `IS`
returns `FALSE` for every option when the union is valueless.

`value??` reports that an option is active. `value?!` reports the valueless
state:

```quxlang
ASSERT(success??);
ASSERT((success?!) == FALSE);
```

Use [`MATCH`](match.md) to inspect or mutate a payload. Union arms use
`CASE option_name`; `TYPE` arms are for variants.

## Valueless policies

Fusion modifiers follow `UNION` or `INLINE_UNION` and precede the opening brace:

```quxlang
::optional_error INLINE_UNION VALUELESS_DEFAULT
{
  .error OPTION I32;
}
```

The state modifiers are:

- `VALUELESS_DEFAULT`: no option is active after default construction. It
  cannot be combined with a `DEFAULT` option.
- `NEVER_VALUELESS`: generated lifecycle operations preserve an active option.
  It cannot be combined with `VALUELESS_DEFAULT`. Generated move operations
  reset the source to the default option, so that option and its required
  payload operations must be available.

Without `NEVER_VALUELESS`, moving from a union can leave the source valueless.
A valueless object remains a valid union object, but it has no payload to bind.
`MATCH` must account for the state through `DEFAULT`, `DEFAULT FAIL`, or the
implicit failure behavior described on the `MATCH` page.

## Generated lifecycle operations

Quxlang can generate copy construction, move construction, assignment, swap,
and destruction by dispatching to the active payload. Availability depends on
the operations provided by the option types.

The following modifiers suppress one generated operation:

- `NO_DEFAULT_COPY`
- `NO_DEFAULT_MOVE`
- `NO_DEFAULT_ASSIGN`
- `NO_DEFAULT_SWAP`

Suppression does not invent a replacement. An operation is unavailable unless
the union declares a suitable user implementation. Generation is demand-driven:
an unusable payload operation matters when the corresponding union operation is
required.

Copying preserves the source and copies its active payload. Moving transfers
the active payload and applies the union's valueless policy to the source.
Assignment replaces the destination state. Swap exchanges active states,
including valueless states; self-swap preserves the object. Destruction destroys
the active payload, if any, exactly once.

## Inline and boxed identity

The inline/boxed choice changes representation, recursive-layout capability,
and how payload storage is owned. It does not change the option names, `IS`
tests, named construction syntax, or `MATCH` surface. Code should normally
choose the representation as part of the type's declared ABI rather than
depending on it at each use site.

See [Variants](variants.md) for type-selected alternatives and [`MATCH`](match.md)
for exhaustive payload handling.
