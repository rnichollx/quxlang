# Overview of `VISIT`

`VISIT` opens a variant and compiles one region for each payload type. It is
useful when the region should use ordinary overload resolution instead of
listing separate [`MATCH`](match.md) arms.

## Visit a variable

An attached block temporarily shadows the variant variable with a reference to
its active payload:

```quxlang
::number_or_count INLINE_VARIANT [I32 DEFAULT, U32];

VAR value number_or_count := 7 AS I32;

VISIT value
{
  print_number(@value value);
}
```

Inside the block, `value` is an `I32` reference in one generated
specialization and a `U32` reference in the other. After the block, `value`
again denotes the variant object.

Ending `VISIT` with a semicolon specializes the rest of the current block:

```quxlang
VISIT value;
print_number(@value value);
```

## Visit an expression

Use `AS` to name the payload of an arbitrary expression:

```quxlang
VISIT read_number() AS number
{
  print_number(@value number);
}
```

The subject is evaluated once. With an attached block, all temporaries made
while evaluating it remain alive through the block. The ordinary semicolon
form keeps only the final temporary variant:

```quxlang
VISIT read_number() AS number;
print_number(@value number);
```

Add `EXTEND` when the entire evaluation context must remain alive through the
rest of the current block:

```quxlang
VISIT EXTEND read_number_with_context() AS number;
print_number(@value number);
```

## `VOID` and valueless variants

A variant containing `VOID` may be visited only with an attached block. When
`VOID` is active, execution skips that block; no payload binding or `VOID`
specialization is formed.

Visiting a valueless variant is undefined behavior. Check `value??` first when
the variant's policy permits a valueless state.

## Reference

See the [`VISIT` Reference](../reference/visit.md) for all five forms,
temporary-lifetime rules, qualification, specialization, and control-flow
constraints.
