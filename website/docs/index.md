# Quxlang

Quxlang (pronounced like "k-whuh-ks-lang" /ˈkwʌks.læŋɡ/) is a systems programming langauge focused on providing fast
cross-platform deterministic and reproducible builds. Quxlang is a proper noun, simliar to Erlang, the language is never called "Qux".

This website documents the current compiler surface with feature-focused pages
and source examples. Quxlang remains under active development and has not yet
made a stable language release, so the current reference is descriptive rather
than a long-term compatibility promise.

!!! warning "NO WARRANTY"
    These docs are provided without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in the software.

## Development Status

Quxlang is in early development stages, and `qxc` can target Windows, MacOS and Linux.
Future support for various BSD, Hurd, and other open source operating systems is planned for
future work.

For native targets using LLVM, it currently supports x86, x64, ARM, ARM64 and z/Architecture(s390).
Other architectures like RISC-V, POWER, SPARC, LoongArch64 and are planned to get future support,
with RISC-V being the priority next target.

For non-native targets, `qxc` currently supports the Cortado backend for generating
executable jar files which can be run using `java -jar <jarfile>`. Note that the
current VMIR to Cortado translation algorithm is extremely inefficient and has no 
optimization passes or escape analysis (and no, the Java JIT compiler will not 
help much here). The inefficiency is mainly due a large number of bad workarounds
to get totally ordered pointers and pointer arithmetic working in a "correct" 
manner in a virtual machine that doesn't support totally ordered pointers.

CLI/CIR support is planned for the future, but there is no current code for this. It 
is expected to perform much better than JVM due to CIR having pointers in the intermediate
representation, which makes translation relatively trivial.

## Other Links

- [Code Repository](https://gitlab.com/rpnx/quxlang)
- [Quxlang Blog](https://quxlang.blog/)

## Website Content

!!! warning "AI Usage"
    This documentation contains a mixture of human written and AI generated content and may contain errors. Report any issues on [Discord](https://discord.gg/Z9qRXXxRtY).

## Overview

- [Getting Started](guide/index.md) explains the source-bundle model and the
  shortest path to a first program.
- [First Program](guide/first-program.md) walks through one complete source file.
- [The `qxcbuild.yml` Overview](overview/qxcbuild-file.md) introduces targets,
  module mappings, build options, and outputs.
- [Language Overview](overview/index.md) gives an example-led tour of every
  language feature.
- [Syntax at a Glance](syntax-examples.md) is a compact lookup page.
- [Design Philosophy](philosophy/overview.md) explains the goals behind
  determinism, value semantics, safety boundaries, and control flow.

## Explore the language

#### Values and computation

Read about [primitive types](overview/primitive-types-and-literals.md),
[arrays](overview/arrays.md),
[references](overview/references.md), [pointers](overview/pointers.md),
[arithmetic operators](overview/arithmetic-operators.md),
[comparison operators](overview/comparison-operators.md), and
[conversions](overview/conversions.md).

#### Functions and reusable code

Read about [functions](overview/functions-and-parameters.md), the explicit
[call argument model](overview/call-arguments.md),
[overloads](overview/overload-resolution.md),
[templates](overview/templates-and-value-parameters.md),
[variadic packs](overview/variadic-packs.md), and
[lambdas](overview/lambdas.md).

#### Control flow and compile-time execution

Read about [conditional statements](overview/conditional-statements.md),
[`WHILE` loops](overview/while-loops.md),
[`LOOP` clauses](overview/loop-statements.md),
[labels](overview/labels-and-goto.md), and
[static evaluation](overview/compile-time-evaluation.md).

#### Data modeling and lifetime

Read about [structures](overview/structs-and-members.md),
[inheritance](overview/inheritance.md),
[constructors and destructors](overview/constructors-and-destructors.md),
[explicit storage lifetime](overview/typed-storage-and-lifetime.md),
[compile-time allocation](overview/constexpr-allocation.md),
[`STATIC` constants](overview/static-compile-time-constants.md),
[`NEW` and `DELETE`](overview/new-and-delete.md),
[enums](overview/enums.md), [flagsets](overview/flagsets.md),
[unions](overview/unions.md), [variants](overview/variants.md), and
[`MATCH`](overview/match.md).

#### Abstraction and configuration

Read about [interfaces](overview/interfaces-and-implementations.md),
[generics](overview/generics.md),
[conditional declarations](overview/availability-and-targets.md),
[privacy](overview/privacy.md), and
[build options](overview/build-options.md).

#### Interoperation, concurrency, and testing

Read about [serialization](overview/serialization.md),
[integer encodings](overview/integer-serialization.md),
[external procedures](overview/external-procedures.md),
[assembly procedures](overview/assembly-procedures.md),
[atomic objects](overview/atomics.md),
[thread-local variables](overview/thread-local-variables.md),
[tests](overview/tests.md), and
[explicit diagnostics](overview/diagnostics-and-failure.md).

## Current documentation boundary

The language reference covers implemented source forms supported by the live
parser and exercised compiler paths. Target-specific gaps, including the lack
of inheritance support in the JVM backend, are stated on the affected feature
pages. Other forward-looking work is intentionally not listed as a current
feature. Low-level VMIR instruction specifications are included in repository
engineering documents rather than as part of the source-language reference.
