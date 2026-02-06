# Quxlang

## Progress status:

You cannot compile _anything_ yet. The compiler _only_ runs things in a _constexpr virtual machine_ at the moment. QXVMIRv2 does not have an LLVM translator at the moment (unlike v1).

It may seem strange to focus on constexpr VM first, but this is where all the _difficult_ parts of the compiler are tested. It also really helps me to avoid debugging middle-end issues when working on the front-end to have a working constexpr VM.


Latest video: https://www.youtube.com/watch?v=pjHL4K-AuKo

## Overview

Quxlang is a systems programming language. It's intended as an improvement over C++ for certain uses.  It is still in
active development but is not yet ready for people to use or try out. Please wait until a later date when there is 
a standard library and more codegen. For now, you can compile the quxlang_gtests and modify the main_test.qx file if
you want to experiment.

The compiler is mainly developed using clang and libc++. Compatibility with GCC tends to vary between commits. I try to make sure it compiles with GCC as well, but there can be a long stretch of commits before GCC compatibility is restored if I break it during development accidentally.

## Dependencies:

Compilers:

* Clang
* GCC (Sometimes works)
* MSVC (Best effort, sometimes works)

Stdlib:

* libc++ (Recommended)
* libstdc++ (Sometimes works, I usually fix it every few weeks)

Build system:

* CMake (required)
* Ninja (recommended)

Other libraries

* YAMLCpp (Suggest using my fork at https://github.com/rnichollx/yaml-cpp, it can be made to compile with the official one but some CMake / CBuild integration doesn't work)
* GTest (For tests)
* Benchmark (For benchmarks; using my fork https://github.com/rnichollx/google-benchmark is suggested, although this mostly only matters if you want to use CBuild/CSetup)
* ~~LLVM v16.0.6~~ (will return later, currently the machine code generator was removed for IRv1 and and IRv2 will reimplement it)

If you follow quick setup guide below, CBuild/CSetup are also required, but it's just a wrapper around CMake, so you don't need it.


## Quick Setup Guide

Install Go, libc++, and clang.

Then run the following commands to install CBuild/CSetup:

```
go install gitlab.com/rpnx/cbuild-go/cmd/cbuild@preview && \
go install gitlab.com/rpnx/cbuild-go/cmd/csetup@preview
```

After that is done, run the following commands inside this project to setup a workspace and build the project:

```bash
csetup dev-init && cd ./dev-workspace && cbuild
```

And/or run the following command to get arguments for the IDE of your choice:

```
csetup get-args  quxlang --config Debug --toolchain system-clang-libcxx 
```

(if you are on MacOS, replace `system-clang-libcxx` with `system-clang`)

(also toolchain detection doesn't work on windows yet, so you may have to provide your own toolchain file...)


```bash
cbuild test
```

Will build all targets and run tests.

```bash
cbuild build # or just `cbuild`
```

Will just build all targets.

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
* Floating point
* Global objects
* Exceptions
* Polymorphism / virtual functions

 
## Note regarding "Qux"

There is another project called "Qux" which is an abandoned/archived programming language project. Quxlang is not related to that project aside from having a similar name.