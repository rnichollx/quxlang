# Quxlang

Quxlang (pronounced like "k-whuh-ks-lang" /ˈkwʌks.læŋɡ/) is a systems programming langauge focused on providing fast
cross-platform deterministic and reproducible builds. Quxlang has two main goals:

* Produce deterministic, reproducible and hermetic builds: given the same sources and compiler version, produce exactly the same output regardless of where the compiler is run.
* Provide the fastest, highest performance systems programming language available. Quxlang should be designed with a mind to ensure that large programs written in Quxlang perform better than equivalent C or C++ programs.

## Progress & Quxlang Today

`qxc` is able to generate working binaries for Windows, MacOS and Linux. Many planned langauge features are still work-in-progress and the edges may be rough. While this is an early work in progress, Quxlang is nonetheless relatively advanced with validated features like:
 
* RAII, constructors, destructors, exception handling & unwinding try/catch etc.
* Fully working templates
* Multiple & virtual polymorphic inhertiance
* Compile generated type-erased generics and interfaces
* Comprehensive compile-time static execution (similar to constexpr)
* Threading, mutexes, atomics, etc.
* Filesystem I/O,
* Routine multiversionsing with program steppings.
* etc.

Compared to C++, Quxlang Release-mode native code performance is strongly competitive with gcc/clang `-O2` and Quxlang can often beat them unless `-march=native` and LTO are enabled.

Compared to Rust, Quxlang changes less semantically from C++, while offering a few minor safety advantages over C++. However, Quxlang and Rust diverge substantially in phillosophy: Rust aims to be safer than C++, Quxlang aims to be better and faster.

Compared to Carbon, Quxlang does not aim to be a "Simpler C++", but rather "Better C++". In terms of semantic features, Quxlang is arguably _more complicated_ than C++, as it implements several features, including e.g. type erasing generics, interfaces, fusion types (discriminated unions) and etc. which are not natively implemented by C++ as part of the core langauge, as well as e.g. making distinctions between _instance pointers_ and _array pointers_.

Major syntax changes may still occur as there is no official release yet.

Please keep in mind that it being possible to generate executables does not mean that they are ready for production use,
as more through testing of the compiler's accuracy is still ongoing.

The most authoritative examples of currently tested and validated syntax are the checked in `.qxs` files in `quxlang/tests/testdata/testbundle` relative to the repo root, with some documentation at
[quxlang.dev](https://quxlang.dev/).

## Overview of Quxlang

Quxlang is a systems programming language, intended as a successor language to C++. The Quxlang compiler, qxc is
a deterministic and reproducible cross-compiler. Quxlang is designed to be similar to C++, but it breaks with C++ in
various areas.

The `qxc` compiler does not accept flags that alter the generated binaries. Optimization options are added to a build
file, currently `qxcbuild.yml`. The `qxc` compiler also does not provide a built-in system library, a system library
needs to be provided in the source bundle instead. This guarantees reproducible compilation results.

Quxlang is generally similar to C++ in philosophy, it strives for negative overhead abstraction, powerful template
metaprogramming, and high performance code with "RAII" abstractions.

Quxlang differs from C++ in several notable ways:

* Quxlang does not have build scripts or configure steps. Quxlang's qxc compiler works on the principle of "Compiler as Build System" against a build file and source directories, which handles multi-module compilation and linking. The compiler is inherently a reproducible cross-compiler. You point qxc at the source bundle, and it will produce the same artifact binaries regardless of where it is run from. Reproducing a Quxlang binary is as simple as checking out the same source code and using the same compiler. qxc does not support the concept of "installed" libraries that break your build when your system updates, it only uses the files in `<bundle path>/sources`. Dependency versions can be easily managed using e.g. git submodules.
* Quxlang doesn't use header files, the entire compiler uses a module based compilation system, this prevents recompiling the same templates many times which is conjectured to improve the compilation time of large projects with many templates.
* The Quxlang language surface is designed to be complete, without relying on hidden compiler magic builtins. The
  compiler and system libraries are treated as separate components.
* Quxlang is generally _not_ designed with ABI compatibility in mind. This is because in Quxlang, we don't compile libraries separately from the main program. This allows the ABI to shift dynamically. For cases where ABI compatibility is required, Quxlang provides IBC (Inter Binary Communication) mechanisms to ensure compatibility, but it is not the default behavior. For example, IBC_STRUCT allows declaring a struct with C layout, but the standard STRUCT declaration will use a compiler-optimized layout that can change between different versions of the Quxlang compiler. Generally speaking, communication across binary boundaries in Quxlang is expected to occur using `IBC_*` types and functions and not ordinary ones.
* Quxlang has a stricter type aliasing model than C++. This makes some types of reinterpret casting allowed in C++
  illegal in Quxlang, but unlocks additional optimization opportunities.
* Trivial type destructors de-initialize their storage, unlike in C++ where they are a no-op. This enables additional optimizations.
* Keywords in Quxlang are always `UPPER_CASE` and identifiers are always `lower_case`. This ensures new keywords can be added without breaking existing code.
* Narrowing casts are not allowed implicitly and require an explicit casting mode to be chosen:
    * `PARTIAL`: Discard extra bits, and/or convert the MSB to/from a sign bit.
    * `ASSUME`: Program behavior is undefined if the value is out of range.
    * `CHECKED`: Runtime fault if the value doesn't fit.
* Certain operations are safe by default, for example, constructed integers are set to the value of 0 instead of
  undefined when default constructed. Although Quxlang aims to be "faster than C++", we nonetheless introduce safer _defaults_ where they have minimal _measured_ performance impacts.

Examples of syntax are in: `quxlang/tests/testdata/testbundle` relative to the repo root, with `quxlang/tests/testdata/testbundle/modules/tests/sources` in particular having the most relevant examples.

## Development

The compiler is mainly developed using clang and libc++. Compatibility with GCC tends to vary between commits. I try to make sure it compiles with GCC as well, but there can be a long stretch of commits before GCC compatibility is restored if I break it during development accidentally.

## System

It's recommended to compile with at least 32GiB RAM and 100GB disk space. Using less RAM (like 16GiB) is possible, but can require reducing the number of compile jobs, particularly for LLVM. You may be unable to run ASAN / TSAN tests with low-RAM configurations.

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

Other RPNX Libraries:

* [RPNX::DataStructures](https://gitlab.com/rpnx/datastructures) - An implementation of several data structures used in the project, like `rpnx::variant`, `rpnx::annex`, `rpnx::sharded_unordered_map` and others.
* [RPNX::QueryGraph](https://gitlab.com/rpnx/querygraph) - This library basically does most of the heavy lifting regarding dyanamic memoization and thread scheduling (this is library is essentially why I am able to write a multi-threaded compiler by myself).
* [RPNX::Metalib](https://gitlab.com/rpnx/metalib) - This library is mostly for adding reflection support without having C++26 reflection. Mostly used as glue for the next library.
* [RPNX::Serailization](https://gitlab.com/rpnx/serialization) - This library is for serializing data structures to/from binary. Used mostly by QueryGraph for inspecting debug compiler graph dumps. Also does JSON/YAML output.
* [RPNX::COW](https://gitlab.com/rpnx/cow) - Copy-on-Write data structure. This really should be folded into RPNX::DataStructures but I'm lazy.
* [RPNX::Cortado](https://gitlab.com/rpnx/cortado) - For generating JVM Bytecode.
* [RPNX::Compress](https://gitlab.com/rpnx/compress) -  Dependency of RPNX::Cortado.

Third-Party libraries

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

## FAQ

### No hardware_destructive_interference_size in namespace std

Either your compiler is too old, or your Linux distribution has for some godforsaken reason decided to add a patch disabling `hardware_destructive_interference_size`. Known issue on Ubuntu. Use Arch Linux instead BTW (Or Arch based distro like Manjaro). OR recompile clang/libc++ from source code. This constant has been in C++ nearly 10 years now so I don't have a lot of sympathy for toolchains without it.

### No `name@version` syntax is allowed

Upgrade to Go 1.22 or newer.

### No `io/fs`

Upgrade to Go 1.22 or newer.

### Error `bash\r`

Fix your git configuration:
```
git config --global core.autocrlf false
git config --global --unset core.eol
```
Then delete and re-clone the repository.

### Clang crashes compiling for Windows

You may need to add `-mllvm -opt-disable=jump-threading,correlated-propagation` to disable buggy optimization passes that crash
the clang compiler when building `qxc.exe`. clang producing linux and macos `qxc` seems unaffected.

### `qxc.exe` is extremely slow on Windows

One may notice that a 3 minute MacOS compile takes over 60 minutes on Windows. This is a known issue on Windows. On Windows, `qxc` spends over 90% of execution time inside the malloc/free implementation functions. Compiling with mimalloc instead of the default allocator provides a more than 10x speed increase to `qxc`, but I don't yet have a clean way to integrate and set this up automatically with `cbuild`/`csetup`. You may also wish to try jemalloc or tcmalloc if you can get them working. qxc is realtively alloc-heavy as the compiler consturcts complex ASTs, and thus strongly benefits from fast allocators. With a good allocator, most of the execution time should be spent in LLVM backend optimization passes.

## Links

* [Quxlang Blog](https://quxlang.blog/)
* [Quxlang Reference](https://quxlang.dev/)
* [Quxlang Vlog](https://www.youtube.com/@Quxlang)

## Note regarding "Qux"

Quxlang is _always_ the complete name, referred to as "Quxlang" (similar to how "lang" is part of the name of "Erlang"), never as "Qux" nor as "QuxLang". The "L" in Quxlang should never be capitalized (unless Quxlang is written using ALL CAPS). There is another project called "Qux" which is an archived programming language project; that is a different project entirely, and Quxlang is not related to that project aside from having a similar name.

## Is Quxlang affiliated with Google?

No. While Ryan Nicholl, the main author of the project is a Google employee, it's not made as part of his duties as a Google employee, and has been granted an IARC comittee exemption by Google. Ryan Nicholl does not work on compilers as part of his work at Google. As such, the entire project is maintained by Ryan Nicholl as an individual and not as a Google employee.
