# Overview of Interfaces and Generics

Quxlang supports dynamic typing primarily through interfaces and generics. Polymorphic types are also supported but are not part of the interface and generic mechanisms.


## Interfaces

Quxlang `INTERFACE` classes are are the most fundamental primitive to implement dynamic typing. 
It is worth noting that Quxlang interfaces are much lower-level primitives than what other languages
like Java or Go call "interfaces".

Languages like C++ use so-called "vtables" as an implementation detail to implement dynamic polymorphism
and virtual dispatch.

In abstract, an `INTERFACE` class could be understood as providing a signature or layout for a
vtable. In actuality, `INTERFACE` is a bit more general than a vtable, as they can potentially
be optimized at link time to use other structures for improved performance.

An interface object is essentially an opaque handle upon which certain declared function signatures can be invoked,
the interface can have any number of implementations, each of which provides the concrete implementations
of the interface functions.

The interface class itself declares the function signatures that are available on the interface, while the
interface objects are opaque pointer-like handles to a specific implementation or possibly a null-interface.

```quxlang
::my_interface INTERFACE
{
    .foo FUNCTION(@ARG I32): I32;
    .bar FUNCTION(@ARG I64): I64;
}
```

In this example, the interface class `::my_interface` declares two function signatures, `.foo` and `.bar`,
which can be invoked on interface objects. An interface can be used like so:

```
::my_function FUNCTION(@inter my_interface, @a I32): I64
{
  RETURN inter.foo(a+1) + inter.bar(64);
}
```

Interfaces may be implemented by an IMPLEMENTATION declaration:

```quxlang
::my_interface_impl IMPLEMENTATION(my_interface) 
{
  ::foo FUNCTION(@ARG I32): I32
  {
    RETURN ARG+1;
  }
  
  ::bar FUNCTION(@ARG I64): I64
  {
    RETURN ARG*2;
  }
}
```

In this manner an `INTERFACE` object can be constructed from an `IMPLEMENTATION` class:

```quxlang
VAR inter my_interface := my_interface_impl;
```

The `inter` object is a handle to the `my_interface_impl` implementation of the `my_interface` interface.

Compared to Java or Go, interfaces differ in that they do not carry around objects or object references, 
an interface object is purely a reference to a collection of function implementations.

The Quxlang `INTERFACE` is the recommended lowest level primitive which can be used to implement vtable
like functionality.

While it is possible to implement interface-like behaviors using pointers to antestatal structures,
this can reduce performance compared to using `INTERFACE` objects.

To understand why consider for example, that the compiler could identify that an interface has only 1 user, 
and inline the implementation at call sites during link-time optimization.

Calls to interfaces can also potentially be inlined in link-time optimization using a binary search though
identified implementors, which often performs much better than table based function pointer dispatch,
especially when there are 8 or less implementors of the interface.

Compiler backends like LLVM are  not sophisticated enough to do these sorts of optimizations, nessecitating 
integration of `INTERFACE` into the language and provinding special VMIR ops for manipulating them,
and frontend optimization and awareness when generating LLVM IR to take advantage of these optimization
oportunities.


## Generics and Generic References

Generic references and generics sit one abstraction level above the interface. The `GENERIC_REF` is best 
explained first. If the `INTERFACE` object is merely reference to a table of functions, the `GENERIC_REF`
can be seen as an interface whose functions have a `THIS` parameter, plus a pointer to an object
to pass to those function.

The `GENERIC_REF` class therefore defines a list of member functions which an implementing object's class must 
implement in order to construct a generic reference object to a given implementing object.

The `GENERIC` class builds upon the `GENERIC_REF`, but with a minor difference. When constructing a 
`GENERIC` object, the object from which the `GENERIC` object is constructed is copied to create a new object
which is then used as the `THIS` parameter for the generic reference object. When the `GENERIC` object is 
likewise copied, that underlying object is likewise copied.

Therefore, unlike `GENERIC_REF` objects, `GENERIC` objects own the underlying object they reference, and such object
shares the lifetime of the `GENERIC` object.

It is therefore acceptable to return a `GENERIC` object constructed from a temporary object, whereas a 
`GENERIC_REF` object constructed from a temporary object would become a dangling reference and trigger
undefined behavior if any of its member functions are called.






