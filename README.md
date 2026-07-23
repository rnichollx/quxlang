# Quxlang

Quxlang (pronounced like "k-whuh-ks-lang" /ˈkwʌks.læŋɡ/) is a systems programming langauge focused on providing fast
cross-platform deterministic and reproducible builds.

## Progress status:

`qxc` is able to compile and link a hello world program for linux targets, but it's not ready for production use.

Additionally, major syntax changes may still occur as there is no official release yet. Some planned features do not
work.

## Overview



Quxlang is a systems programming language, intended as a partial successor language to C++. The Quxlang compiler, qxc is
a deterministic and reproducible cross-compiler. Quxlang is designed to be similar to C++, but it breaks with C++ in
various areas.

The `qxc` compiler does not accept flags that alter the generated binaries. Optimization options are added to a build
file, currently `quxbuild.yaml`. The `qxc` compiler also does not provide a built-in system library, a system library
needs to be provided in the source bundle instead. This guarantees reproducible compilation results.

Quxlang is generally similar to C++ in philosophy, it strives for negative overhead abstraction, powerful template
metaprogramming, and high performance code with "RAII" abstractions.

Quxlang differs from C++ in several notable ways:

* Quxlang does not have build scripts or configure steps. Quxlang's qxc compiler works on the principle of "Compiler as Build System" against a build file and source directories, which handles multi-module compilation and linking. The compiler is inherently a reproducible cross-compiler. You point qxc at the source bundle, and it will produce the same artifact binaries regardless of where it is run from. Reproducing a Quxlang binary is as simple as checking out the same source code and using the same compiler. qxc does not support the concept of "installed" libraries that break your build when your system updates, it only uses the files in `<bundle path>/sources`. Dependency versions can be easily managed using e.g. git submodules.
* Quxlang doesn't use header files, the entire compiler uses a module based compilation system that does caching on the level of individual symbols, this prevents recompiling the same templates many times which is conjectured to improve the compilation time of large projects with many templates.
* The Quxlang language surface is designed to be complete, without relying on hidden compiler magic builtins. The
  compiler and system libraries are treated as separate components.
* Quxlang has a stricter type aliasing model than C++. This makes some types of reinterpret casting allowed in C++
  illegal in Quxlang, but unlocks additional optimization opportunities.
* Trivial type destructors de-initialize their storage, unlike in C++ where they are a no-op. This enables additional optimizations.
* Keywords in Quxlang are always `UPPER_CASE` and identifiers are always `lower_case`. This ensures new keywords can be added without breaking existing code.
* Narrowing casts are not allowed implicitly and require an explicit casting mode to be chosen:
    * `PARTIAL`: Discard extra bits, and/or convert the MSB to/from a sign bit.
    * `ASSUME`: Program behavior is undefined if the value is out of range.
    * `CHECKED`: Runtime fault if the value doesn't fit.
* Certain operations are safe by default, for example, constructed integers are set to the value of 0 instead of
  undefined when default constructed.

## Development

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
* LLVM v22

If you follow quick setup guide below, CBuild/CSetup are also required, but it's just a wrapper around CMake, so you don't need it.

## Quick Setup Guide

TL;DR:

```bash
./configure && make test
```

Install CMake, Ninja, Go, libc++, and clang.

Then run the following commands to install CBuild/CSetup:

```
go install gitlab.com/rpnx/cbuild-go/cmd/cbuild@preview && \
go install gitlab.com/rpnx/cbuild-go/cmd/csetup@preview
```

After that is done, run the following commands inside this project to setup a workspace and build the project:

```bash
./configure && make test
```

If you want to mess with the code, the actual build is done with CBuild and configured with CSetup. The Makefile is just
a wrapper that invokes CSetup/CBuild with the correct arguments for your system. You can look at `misc/build` for more details.

## Status

Currently, some layers are mostly working. If you run the quxlang_gtests func_gen test, the compiler should run several
tests against the compiler API.

Known gaps and remaining work are tracked in [docs/TODO.md](docs/TODO.md).

What works or mostly works:

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

## Note regarding "Qux"

Quxlang is _always_ the complete name, referred to as "Quxlang" (similar to how "lang" is part of the name of "Erlang"), never as "Qux" nor as "QuxLang". The "L" in Quxlang should never be capitalized (unless Quxlang is written using ALL CAPS). There is another project called "Qux" which is an archived programming language project; that is a different project entirely, and Quxlang is not related to that project aside from having a similar name.
