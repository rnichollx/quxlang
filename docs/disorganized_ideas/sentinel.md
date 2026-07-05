# SENTINEL keyword

Add a `SENTINEL` keyword for pointer types.


Example:

```quxlang
::list_update SENTINEL ->foo;
```

Upon declaring a SENTINEL, the compiler creates a unique pointer which cannot alias or
compare equal with any object of the pointee type or an inheritance-compatible pointer
type.

If two sentinel pointers of different types are cast to VOID, it is unspecified whether
or not they compare equal unless the sentinel is declared GLOBAL, for example:

```quxlang
::global_sentinel SENTINEL GLOBAL ->foo;
``` 

A sentinel declared global cannot compare equal to any other sentinel pointer,
even if it's first cast to VOID.

Dereferencing a sentinel pointer is undefined behavior.

