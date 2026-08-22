# Constructors and Destructors

Constructors and destructors are reserved member declarations named
`.CONSTRUCTOR` and `.DESTRUCTOR`.

## Ordinary construction

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
}

VAR named point :(@x 3, @y 4);
```

Constructor arguments use the same named and positional rules as functions.
Named construction is preferred for ordinary object APIs.

Positional constructors remain available when position is an intentional part
of the type's interface:

```quxlang
::coordinate_pair STRUCT
{
  .x VAR I32;
  .y VAR I32;

  .CONSTRUCTOR FUNCTION(%x I32, %y I32)
  {
    .x := x;
    .y := y;
  }
}

VAR positional coordinate_pair :[3, 4];
```

## Field delegates

The `:>` list constructs fields before the constructor body:

```quxlang
::owner STRUCT
{
  .member VAR point;

  .CONSTRUCTOR FUNCTION(@x I32, @y I32) :> .member:(@x x, @y y)
  {
  }
}
```

A field not explicitly delegated follows its applicable default construction
rule.

## Conversion constructors

Reserved named parameters identify conversion categories:

```quxlang
.CONSTRUCTOR FUNCTION(@OTHER CONST& owner)
{
  .member := OTHER.member;
}

.CONSTRUCTOR FUNCTION(@OTHER TEMP& owner)
{
  .member <-> OTHER.member;
}
```

`@OTHER` is the ordinary source category used for implicit, copy, or move
construction depending on its type and qualifier. Other supported category
names are `@EXPLICIT`, `@REINTERPRET`, `@PARTIAL`, `@ASSUME`, `@CHECKED`, and
`@APPROXIMATE`; they correspond to explicit [conversion modes](conversions.md).

## Destruction

```quxlang
.DESTRUCTOR FUNCTION()
{
  IF (.allocation??)
  {
    release(@allocation .allocation);
  }
}
```

Local objects are destroyed automatically when their lifetime ends. Arrays
destroy their live elements, and control flow may not bypass required
construction or destruction.

The compiler supplies eligible default, copy, move, assignment, swap, and
destruction operations unless struct modifiers or user declarations disable or
replace them.
