# Overview of Structures

A `STRUCT` groups stored fields with member functions and nested declarations.
It is a value type: constructing, copying, moving, and destroying the structure
also handles its fields.

## Define fields and members

```quxlang
::point STRUCT
{
  .x VAR I32;
  .y VAR I32;

  .sum FUNCTION() CONST: I32
  {
    RETURN .x + .y;
  }

  ::origin FUNCTION(): point
  {
    RETURN point();
  }
}
```

A leading `.` declares or accesses an instance member. Inside `.sum`, `.x` is
the `x` field of `THIS`. A leading `::` declares a name nested in `point` but
not attached to an instance.

## Control receiver access

```quxlang
.read FUNCTION() CONST: I32
{
  RETURN .x;
}

.reset FUNCTION() MUT
{
  .x := 0;
  .y := 0;
}
```

The suffix expresses the receiver category. `CONST` permits read-only access,
while `MUT` permits mutation. The compact suffix uses the same semantic model
as an explicit `@THIS ... THISTYPE` parameter.

## Select structure behavior

```quxlang
::unique_owner STRUCT MOVE_ONLY
{
  .allocation VAR MUT->BYTE;
}
```

Structure modifiers can constrain generated special operations or request
contracts such as `ANTESTATAL`, `SERIALOID`, `NONSTATIC`, and `STRINGLIKE`.
`POLYMORPHIC` and `VIRTUAL_POLYMORPHIC` opt a structure into runtime
inheritance behavior, while `FINAL` prevents further derivation. See
[Inheritance](inheritance.md) for base declarations, virtual functions, and
RTTI casts.
Use `IBC_STRUCT` instead of `STRUCT` when a binary interface requires
C-compatible field order and padding.

## Reference

See the [Structures Reference](../reference/structs-and-members.md) for all
receiver categories, every modifier, generated-operation effects, nesting, and
the `IBC_STRUCT` layout contract.
