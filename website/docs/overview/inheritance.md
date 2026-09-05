# Overview of Inheritance

Inheritance lets one `STRUCT` contain base subobjects whose fields and member
functions are available through the derived object. It supports ordinary,
multiple, and virtual bases, and it can add runtime polymorphism when a struct
explicitly opts in.

!!! warning "JVM backend"
    Currently, inheritance is supported only for native code. Inheritance is
    not yet implemented by the Cortado JVM backend. Programs that
    reach inheritance construction, conversion, dispatch, RTTI, or destruction
    operations are rejected while lowering to the JVM target.

## Reuse a base subobject

Declare a named base with `BASE`:

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

Inherited member lookup makes `item.value` available directly. The selector
`item.stored` explicitly projects the `named_value` base subobject. Named
selectors are especially useful with multiple or repeated bases.

When a struct has exactly one direct nonvirtual base, the base may be anonymous:

```quxlang
::special_value STRUCT
{
  .BASE named_value;
  .category VAR I32;
}
```

## Opt in to virtual dispatch

A struct must say `POLYMORPHIC` before it can introduce or override virtual
functions:

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

The argument is statically an `animal`, but the call selects `dog::.speak`.
Use `VIRTUAL(FINAL)` or `VIRTUAL(OVERRIDE, FINAL)` to stop later overrides, and
`VIRTUAL(PURE)` to declare an abstract slot without a body.

## Convert through a hierarchy

Pointers and references convert implicitly from a derived struct to a unique
base subobject. A checked downcast or cross-cast uses `AS DYNAMIC`:

```quxlang
VAR value dog;
VAR base_pointer MUT->animal := value<-;
VAR dog_pointer MUT->dog := base_pointer AS DYNAMIC MUT->dog;

ASSERT(dog_pointer??);
```

A dynamic cast returns null when the runtime object has no unique target
subobject. Dynamic inheritance casts are pointer-only; inheritance does not add
implicit object slicing.

## Recover a known complete type

When you know the complete object is exactly `dog`, use the built-in
`AS UNCHECKED_STATIC_DOWNCAST`:

```quxlang
VAR value dog;
VAR base_pointer MUT->animal := value<-;
VAR dog_pointer MUT->dog :=
  base_pointer AS UNCHECKED_STATIC_DOWNCAST MUT->dog;
ASSERT(dog_pointer == value<-);

VAR base_reference MUT& animal := value;
VAR dog_reference MUT& dog :=
  base_reference AS UNCHECKED_STATIC_DOWNCAST MUT& dog;
```

The cast also works for nonpolymorphic structs and for references. It performs
no runtime type check: casting an object whose complete type is anything other
than the destination struct is undefined behavior, including a further-derived
type. The destination hierarchy must contain exactly one source base subobject;
multiple nonvirtual copies cause a compilation error. Shared virtual bases
count once. The cast cannot be overloaded.

## Inspect polymorphism and dynamic type

`TYPE_IS_POLYMORPHIC(T)` tests the declared type at compile time.
`DYNAMIC_TYPE_OF(pointer)` produces the actual object's `TYPE_INDEX`:

```quxlang
ASSERT(TYPE_IS_POLYMORPHIC(animal));
ASSERT(TYPE_IS_POLYMORPHIC(dog));

VAR pet dog;
VAR pointer CONST->animal := pet<-;
ASSERT(DYNAMIC_TYPE_OF(pointer) == TYPE_INDEX_OF(dog));
```

Dynamic type lookup requires a readable pointer to a `POLYMORPHIC` or
`VIRTUAL_POLYMORPHIC` struct. A pointer to an ordinary struct is a compilation
error; a null pointer causes undefined behavior. See
[Type Queries](../reference/type-queries-and-deduction.md#dynamic-type-identity)
for lifetime-phase behavior and the full query contracts.

## Share a virtual base

Use `VIRTUAL_POLYMORPHIC` and a named `VIRTUAL_BASE` when several paths must
share one base subobject:

```quxlang
::root STRUCT VIRTUAL_POLYMORPHIC
{
  .value VAR I32;
}

::left_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared_root VIRTUAL_BASE root;
}

::right_branch STRUCT VIRTUAL_POLYMORPHIC
{
  .shared_root VIRTUAL_BASE root;
}

::diamond STRUCT VIRTUAL_POLYMORPHIC
{
  .left BASE left_branch;
  .right BASE right_branch;
}
```

The complete `diamond` object contains one shared `root`. Virtual-base
construction uses distinct full-object and subobject constructor entries; the
technical reference describes both the explicit pair and the shorter
`.CONSTRUCTOR` template form.

## Destruction and generated operations

Polymorphic structs have a virtual destructor unless their `.DESTRUCTOR`
declaration is tagged `NONVIRTUAL`. Consequently, `DELETE` through a
polymorphic base pointer destroys the complete runtime object. A nonvirtual
destructor is an unchecked exact-type contract; deleting a derived object
through such a base pointer is undefined behavior.

Copy and move operations remain statically selected rather than becoming
virtual. A polymorphic struct is not implicitly a datatype, so the compiler
does not generate equality, three-way comparison, serialization,
deserialization, or swap for it.

## Reference

See the [Inheritance Reference](../reference/inheritance.md) for base
declaration rules, member ambiguity, virtual modifiers, constructor forms,
dynamic casts, destruction order, and target restrictions.
