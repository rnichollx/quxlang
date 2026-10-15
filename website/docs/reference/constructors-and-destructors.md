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

## The `ARG` construction argument

A class call with one bare argument, such as `owner(value)`, supplies `@ARG`.
Constructor selection first attempts the original argument name. If no viable
constructor accepts that call, selection tries `@EXPLICIT`, then `@OTHER`.
Each category uses ordinary overload resolution; ambiguity is a compilation
error. A named `@ARG` argument follows the same order. Allocation, placement,
and delegated constructor calls use the same selection rules. An already
supplied `@EXPLICIT` or `@OTHER` argument prevents fallback from replacing that
argument. `VAR value T := expression` uses `@OTHER` directly.

```quxlang
::owner STRUCT
{
  .value VAR I32;

  .CONSTRUCTOR FUNCTION(@ARG:value I32)
  {
    .value := value;
  }
}

VAR object owner := owner(7);
```

An `@ARG` constructor participates in direct construction. It does not provide
an implicit conversion or an `AS` conversion. For example, `std::thread(action)`
uses its `@ARG` constructor, while `action AS std::thread` is rejected.

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

Base subobjects use the same delegate list. A named direct base is selected by
its base selector; the sole anonymous base is selected by its base type.
Virtual bases and `VIRTUAL_POLYMORPHIC` full-object/subobject constructor forms
are specified in [Inheritance](inheritance.md).

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

For inherited structs, construction includes bases before fields, and
destruction processes fields and bases in reverse order. Polymorphic destructor
dispatch, `NONVIRTUAL`, and the generated-operation boundary for polymorphic
structs are specified in [Inheritance](inheritance.md).

## Exceptional exits

If construction throws, already constructed subobjects are cleaned up. The
incomplete object does not receive a complete-object destructor call.
Destructors, including generated subobject destruction, are implicitly
`NOEXCEPT`. A destructor may catch exceptions internally; an exception that
escapes its boundary terminates execution. See
[Exception Handling](exceptions.md#cleanup-and-noexcept) for cleanup and
nested-handler rules.
