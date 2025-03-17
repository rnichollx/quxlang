# Quxlang

Quxlang is a systems programming language. It's intended as an improvement over C++ for certain uses.

It is still in active development and is not "usable" yet.

I am compiling with something like:

```
-D LLVM_ROOT=/Users/rnicholl/Dev/llvm-root -D CMAKE_CXX_STANDARD=20 -D Boost_ROOT=/Users/rnicholl/Dev/boost_1_81_0 -D CMAKE_OSX_SYSROOT=/Users/rnicholl/Dev/MacOSX13.1.sdk -DGTest_DIR=/Users/rnicholl/Dev/googletest/gtest-export/lib/cmake/GTest -DCMAKE_CXX_FLAGS="-Wfatal-errors"
```

Currently, the compiler is not complete enough to "work" and all development is done with unit tests.

## Dependencies:

* YAMLCpp
* GTest
* Boost (for demangle)
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
* Type lookup in specific scenarios
* Pointer arithmetic
* Array and wildcard pointers
 
## Note regarding "Qux"

There is another project called "Qux" which is an abandoned/archived programming language project. Quxlang is not related to that project aside from having a similar name.