# Generics (implementation)
 
Generics work like interfaces, but they define operations that are allowed on objects rather than pure function tables. This argument in the generic interface must be explictly provided and no implict this (due to implicit const/non-const behavior), however it can be implicit in the implementing object.

By default, generics are COMPARABLE and COPYABLE.

```quxlang
::foobar GENERIC {
  .baz FUNCTION(@THIS CONST& THISTYPE, @ARG I32);
}
```

Generics must implement the listed member functions.

A generic type may be declared INCOMPARABLE and/or MOVE_ONLY.

```quxlang
::not_copyable GENERIC MOVE_ONLY {
   <...>
}
```

When a generic is compared, it checks if the value's current types are equal, if they are equal, it uses the internal operator<=> or operator== of the underlying type. Otherwise, if they are not equal types, some total order is returned.

Also implement GENERIC_REF,

```
::foobar_ref GENERIC_REF { 
  <... methods>
}
```

This behaves like GENERIC, except that it doesn't copy the object nor allocate storage, only carrying a pointer/reference to the object and its interfaces. Generic ref can be declared const:

```
::foobar_const_ref GENERIC_REF CONST {
   <... methods>
}
```

Like readonly constants, generics are implemented as struct types with their own class category.

Fundamentally, generics have two hidden members, `__INTERFACE_VAL` and `__VALUE`.

The type of `__INTERFACE_VAL` is `<generic>::__INTERFACE` and the type of `__VALUE` is `->VOID`.

The generated `__INTERFACE` is an interface type that contains methods. It will have similar methods but `@THIS` references are replaced with `->VOID` or `CONST->VOID`, remapped from `@THIS` to `@GENERIC_THIS`. etc.

For example:

```quxlang
::foo_generic GENERIC {
  .bar FUNCTION(@THIS CONST& THISTYPE, @val I32);
  .baz FUNCTION(@THIS & THISTYPE, @val I32, @val2 I64);
}
```

Produces an interface that contains in part:

```quxlang
::foo_generic::__INTERFACE INTERFACE {
  .bar FUNCTION(@GENERIC_THIS CONST-> VOID, @val I32);
  .baz FUNCTION(@GENERIC_THIS -> VOID, @val I32, @val2 I64);
}
```

The functions are generated approximately as follows:

```quxlang

::foo_generic GENERIC /*implementation*/ {
  .bar FUNCTION(@THIS CONST& THISTYPE, @val I32)
  {
    RETURN .__INTERFACE.bar(.__VALUE AS CONST-> VOID, PASS(val));
  }
  
  .baz FUNCTION(@THIS &THISTYPE, @val I32, @val2 I64)
  {
    RETURN .__INTERFACE.baz(.__VALUE AS -> VOID, PASS(val), PASS(val2));
  }
}
```

A generic may declare `IMPLEMENTS` to implement another interface. Each method in the implementee is copied into the implementor.

```quxlang
:foo_generic GENERIC {
  IMPLEMENTS COPYABLE_INTERFACE; // implied unless opt-out.
  IMPLEMENTS COMPARABLE_INTERFACE; // implied unless opt-out.
}
```

`COPYABLE_INTERFACE` is a global compiler built-in interface that uses `RUNTIME_MODULE::DEFAULT_ALLOCATOR`:

```quxlang
/* built-in */ COPYABLE_INTERFACE INTERFACE 
{
   .COPY_CONSTRUCT FUNCTION(@GENERIC_OTHER CONST->VOID) : ->VOID;
   .MOVE_CONSTRUCT FUNCTION(@GENERIC_OTHER TEMP->VOID) : ->VOID;
}
```

```quxlang
/* built-in */ COMPARABLE_INTERFACE INTERFACE 
{
   .COMPARE FUNCTION(@RHS CONST->VOID, @LHS CONST->VOID) : ORDER;
   .COMPARE_EQ FUNCTION(@RHS CONST->VOID, @LHS CONST->VOID) : BOOL;
}
```

All generics also implement `GENERIC_INTERFACE`:

```quxlang
/* built-in */ GENERIC_INTERFACE INTERFACE 
{
   .CURRENT_TYPE FUNCTION(): TYPE_INDEX;
   .DELETE FUNCTION(@GENERIC_THIS ->VOID);
}
```

