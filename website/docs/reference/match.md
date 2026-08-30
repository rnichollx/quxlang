# `MATCH`

`MATCH` is a statement that observes one union or variant value, chooses an arm,
and optionally binds a reference to the active payload. Union arms select named
options with `CASE`; variant arms select alternative types with `TYPE`.

## Subject evaluation

```text
MATCH expression [ AS binding | SHADOW ] { clauses }
```

The subject may be an arbitrary expression and is evaluated exactly once. It
must have a union or variant type. `MATCH` does not produce an expression value.

```quxlang
MATCH read_result() AS payload
{
  CASE success { consume(payload); }
  CASE failure AS error { report(error); }
}
```

The header `AS` name is available only in payload-bearing arms. It is absent in
a `VOID` arm and in `DEFAULT`, because those paths do not identify a payload.

## Matching unions

Each union arm uses an option name declared by that union:

```quxlang
::parse_result INLINE_UNION
{
  .success OPTION DEFAULT I32;
  .failure OPTION STRING;
}

MATCH result AS payload
{
  CASE success
  {
    ASSERT(payload >= 0);
  }
  CASE failure AS message
  {
    log(message);
  }
}
```

`CASE` cannot be used for a variant, and `TYPE` cannot be used for a union. An
unknown union option is rejected. Every union option must be handled by an
unguarded arm, by a complete guarded sequence, or by `DEFAULT`.

## Matching variants

A variant selector is the exact alternative type:

```quxlang
::number INLINE_VARIANT [I32 DEFAULT, F64, VOID];

MATCH value AS payload
{
  TYPE I32 { payload++; }
  TYPE F64 AS floating { floating := 0.0; }
  TYPE VOID { }
}
```

The selected type must occur in the variant declaration. Payload bindings for
non-`VOID` alternatives are references to the contained object, so the
binding's access follows the matched subject. Mutating a binding obtained from
a mutable subject mutates the contained value.

## Arm-local bindings

`AS name` after a selector supplies an arm-local name:

```quxlang
MATCH result AS payload
{
  CASE success { use(payload); }
  CASE failure AS error { report(error); }
}
```

An arm-local binding replaces the header binding in that arm; both names are
not simultaneously visible. A `VOID` selector cannot declare a payload binding.

`SHADOW` is shorthand for binding the payload under the subject's own name:

```quxlang
MATCH value SHADOW
{
  TYPE I32 { value++; }
  TYPE VOID { }
}
```

`SHADOW` requires the subject to be one bare identifier. Parenthesized names,
member expressions, calls, and other compound subjects are rejected. The
shadowing binding exists only inside the selected arm; after the statement,
the original name again denotes the union or variant object.

## Guards and `OTHERWISE`

`WHERE` adds a Boolean guard to a selector. Arms are considered in source order
and guards are evaluated only after their selector matches.

```quxlang
MATCH value AS payload
{
  TYPE I32 WHERE payload < 0 { handle_negative(payload); }
  TYPE I32 WHERE payload == 0 { handle_zero(); }
  TYPE I32 OTHERWISE { handle_positive(payload); }
  TYPE VOID { handle_empty(); }
}
```

`OTHERWISE` is the final arm for that same selector. It is selected when the
selector matches and all earlier `WHERE` arms for it are false. A guarded
selector requires a following `OTHERWISE` arm unless a `DEFAULT` clause covers
the remaining state. There can be only one `OTHERWISE` for a selector, and no
later `WHERE` arm may follow it.

An unqualified arm is unconditional for its selector and therefore completes
coverage for that selector.

## `DEFAULT`

`DEFAULT` catches a state not selected by an earlier arm. It has two forms:

```quxlang
DEFAULT
{
  recover();
}

DEFAULT FAIL;
```

Only one `DEFAULT` is permitted, and it must be the final clause.
`DEFAULT FAIL;` makes selection of that path an explicit execution failure.

A valueless union or variant is handled by `DEFAULT` when one is present. If a
potentially valueless type has complete alternative coverage but no `DEFAULT`,
matching a valueless object fails at execution time. Types declared
`NEVER_VALUELESS` do not have that state.

## Coverage and duplicate rules

The compiler checks the declaration behind the subject:

- every `CASE` name or `TYPE` alternative must exist;
- union and variant selector forms cannot be mixed;
- every declared option or alternative must have complete coverage or fall
  through to `DEFAULT`;
- a selector cannot have competing unconditional coverage;
- guarded sequences must terminate with `OTHERWISE` unless `DEFAULT` handles
  their remainder;
- `DEFAULT` must be unique and last.

These checks use the full union or variant declaration, not only the state
known at a particular call site.

## Generated paths

All type-correct arms are part of the generated function even when a particular
compile-time execution selects only one of them. An unselected arm can therefore
remain a lowering dependency. A `PANIC` inside an arm terminates that arm; code
after the terminator is not reachable from it.

See [Unions](unions.md) and [Variants](variants.md) for construction, valueless
states, `IS`, `ISA`, and `UNWRAP`. See [`VISIT`](visit.md) when one common
region should be specialized over every non-`VOID` variant alternative.
