# Quxlang

## Progress update:

Latest progress update: https://www.youtube.com/watch?v=pjHL4K-AuKo

## Overview

Quxlang is a systems programming language. It's intended as an improvement over C++ for certain uses.  It is still in
active development but is not yet ready for people to use or try out. Please wait until a later date when there is 
a standard library and more codegen. For now, you can compile the quxlang_gtests and modify the main_test.qx file if
you want to experiment.

## Dependencies:

* YAMLCpp
* GTest
* LLVM v16.0.6

## Status

Currently, some layers are mostly working. If you run the quxlang_gtests func_gen test, the compiler should run several
tests against the compiler API.

What works:

* Classes
* Constructors / destructors (mostly)
* Operator overloading
* Function calls
* Functions, references
* Instance pointers
* Integer ops (add/subtract, etc.)
* If statement, while statement
* Variable declarations
* Assignment statements
* CONST&, MUT&, TEMP& etc. references.
* Constexpr evaluation of expressions that result in bool
* Pointer arithmetic
* Array and wildcard pointers
* Using multiple modules together

What doesn't work:

* Compound assignments (e.g. `+=`, `-=`, etc.)
* For loops
* Non-constexpr codegen / linking
* Linking with C code
* Floating point
* Strings
* Global objects
* Exceptions
* Polymorphism / virtual functions
* Named arguments in certain contexts.

 
## Note regarding "Qux"

There is another project called "Qux" which is an abandoned/archived programming language project. Quxlang is not related to that project aside from having a similar name.