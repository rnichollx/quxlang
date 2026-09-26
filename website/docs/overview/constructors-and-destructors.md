# Overview of Constructors and Destructors

Constructors establish a value's initial state. Destructors release resources
when that value's lifetime ends. Quxlang declares them as reserved structure
members named `.CONSTRUCTOR` and `.DESTRUCTOR`.

## Named-argument construction

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

VAR location point :(@x 3, @y 4);
```

Constructor calls follow the ordinary named-argument rules. A constructor can
also declare positional `%` parameters when position is deliberately part of
the interface.

## Field initialization delegates

A `:>` delegate supplies constructor arguments for a field:

```quxlang
::owner STRUCT
{
  .member VAR point;

  .CONSTRUCTOR FUNCTION(@x I32, @y I32) :> .member:(@x x, @y y)
  {
  }
}
```

Delegated fields are constructed before the constructor body. Other fields use
their applicable default construction.

Inheritance uses the same delegate list for base subobjects. A named base uses
its selector, and virtual bases are owned by a complete
`VIRTUAL_POLYMORPHIC` object. See [Inheritance](inheritance.md) for ordinary
base delegates and the full-object/subobject constructor forms used with
virtual inheritance.

## Resource release

```quxlang
.DESTRUCTOR FUNCTION()
{
  IF (.allocation??)
  {
    release(@allocation .allocation);
  }
}
```

Local values are destroyed automatically at the end of their lifetime.
Quxlang also generates eligible default, copy, and move operations; structure
modifiers and explicit declarations can suppress or replace them.

Conversion constructors use reserved parameters such as `@OTHER`, `@CHECKED`,
or `@REINTERPRET`. See [Conversions](conversions.md).

## Reference

See the [Constructors and Destructors Reference](../reference/constructors-and-destructors.md)
for positional forms, delegates, conversion categories, implicit special
operations, array destruction, and control-flow restrictions. Inheritance
construction and destruction rules are specified separately in the
[Inheritance Reference](../reference/inheritance.md).
