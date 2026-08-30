# Structures

A `STRUCT` is a value type with fields, member functions, and nested
declarations:

```quxlang
::point STRUCT
{
  .x VAR I32;
  .y VAR I32;

  .CONSTRUCTOR FUNCTION(@x I32, @y I32)
  {
    .x := x;
    .y := y;
  }

  .sum FUNCTION() CONST: I32
  {
    RETURN .x + .y;
  }

  ::origin FUNCTION(): point
  {
    RETURN point(@x 0, @y 0);
  }
}
```

- `.x`, `.y`, and `.sum` are instance members.
- `.x` in a member body is shorthand for the member of `THIS`.
- `::origin` is nested in `point` but receives no object.

## Receiver qualifiers

A member function may place `MUT`, `CONST`, `WRITE`, or `TEMP` after its
parameter list:

```quxlang
.read FUNCTION() CONST: I32
{
  RETURN .x;
}
```

The suffix reuses the ordinary parameter model. `FUNCTION() CONST` is
semantically the same receiver form as an explicit
`@THIS CONST& THISTYPE` parameter, while keeping the common member spelling
compact. `THIS` names the current object and `THISTYPE` names its formal owner
type.

## Struct modifiers

Modifiers appear between `STRUCT` and its body. The current set includes
`MOVE_ONLY`, `NOT_COPYABLE`, `NO_IMPLICIT_DEFAULT_CONSTRUCTOR`,
`NO_IMPLICIT_CONSTRUCTORS`, `NO_IMPLICIT_ASSIGNMENT`, `NO_IMPLICIT_COPY`,
`ANTESTATAL`, `SERIALOID`, `NONSTATIC`, `STRINGLIKE`, `POLYMORPHIC`,
`VIRTUAL_POLYMORPHIC`, and `FINAL`.

```quxlang
::unique_owner STRUCT MOVE_ONLY
{
  .allocation VAR MUT->BYTE;
}
```

The modifiers constrain generated special functions or declare specialized
static/serialization contracts. See
[Constructors and destructors](constructors-and-destructors.md),
[Inheritance](inheritance.md),
[`STATIC` Compile-Time Constants](static-compile-time-constants.md),
and [Serialization](serialization.md).

## `IBC_STRUCT`

An ordinary `STRUCT` may reorder fields to optimize layout. `IBC_STRUCT`
preserves C-compatible field order and padding for external layouts:

```quxlang
::wire_pair IBC_STRUCT
{
  .left VAR U32;
  .right VAR U32;
}
```

Use `IBC_STRUCT` at a binary boundary, not merely to force a preferred internal
layout.
