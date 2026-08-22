# Overview of References

A reference gives non-owning access to an existing object. Its qualifier says
whether that access is mutable, read-only, expiring, or output-only.

```quxlang
VAR value I32 := 7;
VAR mutable_reference MUT& I32 := value;
VAR constant_reference CONST& I32 := value;

mutable_reference := 8;
ASSERT(constant_reference == 8);
```

## Reference qualifiers

- `MUT& T` reads and modifies an existing `T`.
- `CONST& T` reads without modifying through that reference.
- `TEMP& T` marks an expiring source that may be moved from.
- `WRITE& T` is an output destination whose previous value is not input.
- `AUTO& T` deduces an incoming reference qualifier.

Use `WRITE&` for pure output parameters:

```quxlang
::split FUNCTION(@value I32, @high WRITE& I32, @low WRITE& I32)
{
  high := value / 10;
  low := value % 10;
}
```

## Forwarding and returned references

`AUTO& AUTO` deduces both qualifier and target type. `FORWARD` preserves the
deduced category:

```quxlang
::identity FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

A function can return a reference to existing storage:

```quxlang
::box STRUCT
{
  .value VAR I32;
  .get FUNCTION(): MUT& I32 { RETURN .value; }
}
```

References do not own or extend the lifetime of their targets. Use a pointer
when the association may be null.

## Complete technical rules

See the [References Reference](../../reference/references.md) for binding and
qualification rules, output references, deduction patterns, forwarding,
returned-reference lifetime, and conversion to pointer form.

