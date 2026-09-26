# Overview of Inheritance

Inheritance lets a structure extend a base structure and be used through the
base type. A derived object contains its base subobject, and inherited members
can be used directly:

```quxlang
::named_value STRUCT
{
  .value VAR I32;
}

::bounded_value STRUCT
{
  .stored BASE named_value;
  .maximum VAR I32;
}

VAR item bounded_value;
item.value := 7;
item.maximum := 10;
ASSERT(item.stored.value == 7);
```

The selector `item.stored` names the actual base subobject. Its fields occupy
storage inside `item`; inheritance does not create a separate allocation.

A data member represents containment without substitutability. A base
subobject allows a derived value to be accepted through a base reference and
participates in inherited member lookup.

## Runtime polymorphism through virtual functions

`POLYMORPHIC` enables virtual member functions:

```quxlang
::animal STRUCT POLYMORPHIC
{
  .speak FUNCTION() CONST VIRTUAL: I32
  {
    RETURN 1;
  }
}

::dog STRUCT POLYMORPHIC
{
  .BASE animal;

  .speak FUNCTION() CONST VIRTUAL(OVERRIDE): I32
  {
    RETURN 2;
  }
}

::hear FUNCTION(@value CONST& animal): I32
{
  RETURN value.speak();
}

VAR pet dog;
ASSERT(hear(@value pet) == 2);
```

The call through the `animal` reference is dispatched according to the
object's dynamic type. Polymorphism adds runtime type metadata and indirect
calls for virtual functions. Nonvirtual members retain static dispatch.

## Shared base subobjects in a diamond

Multiple inheritance can repeat a base subobject. `VIRTUAL_POLYMORPHIC` and
`VIRTUAL_BASE` describe a hierarchy where several paths share one base:

```quxlang
::root STRUCT VIRTUAL_POLYMORPHIC
{
  .value VAR I32;
}

::left_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared VIRTUAL_BASE root;
}

::right_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared VIRTUAL_BASE root;
}
```

A virtual base provides one shared base subobject when multiple inheritance
paths reach the same base type. Virtual inheritance requires more complex
object layout and construction than ordinary inheritance.

Inheritance currently targets native code; the Cortado JVM backend does not
lower inheritance operations. The [Inheritance Reference](../reference/inheritance.md)
specifies base declaration forms, ambiguity, casts, virtual modifiers,
construction, destruction, generated operations, and target restrictions.
