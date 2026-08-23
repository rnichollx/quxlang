# Quxlang

Quxlang is a systems programming language built around deterministic source
bundles, whole-program compilation, value semantics, explicit lifetime, and
cross-platform code generation.

This website documents the current compiler surface with feature-focused pages
and source examples. Quxlang remains under active development and has not yet
made a stable language release, so the current reference is descriptive rather
than a long-term compatibility promise.

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

## Warranty

These docs are provided without warranty of any kind, express or implied, including but not limited to the warranties of merchantability, fitness for a particular purpose and noninfringement. In no event shall the authors or copyright holders be liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, arising from, out of or in connection with the software or the use or other dealings in the software.

Documentation may contain errors or be out of date with the current compiler status.

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

Read about [primitive types](overview/language/primitive-types-and-literals.md),
[arrays](overview/language/arrays.md),
[references](overview/language/references.md), [pointers](overview/language/pointers.md),
[arithmetic operators](overview/language/arithmetic-operators.md),
[comparison operators](overview/language/comparison-operators.md), and
[conversions](overview/language/conversions.md).

#### Functions and reusable code

Read about [functions](overview/language/functions-and-parameters.md), the explicit
[call argument model](overview/language/call-arguments.md),
[overloads](overview/language/overload-resolution.md),
[templates](overview/language/templates-and-value-parameters.md),
[variadic packs](overview/language/variadic-packs.md), and
[lambdas](overview/language/lambdas.md).

#### Control flow and compile-time execution

Read about [conditional statements](overview/language/conditional-statements.md),
[`WHILE` loops](overview/language/while-loops.md),
[`FOR` clauses](overview/language/for-loops.md),
[labels](overview/language/labels-and-goto.md), and
[static evaluation](overview/language/compile-time-evaluation.md).

#### Data modeling and lifetime

Read about [structures](overview/language/structs-and-members.md),
[constructors and destructors](overview/language/constructors-and-destructors.md),
[explicit storage lifetime](overview/language/typed-storage-and-lifetime.md),
[compile-time allocation](overview/language/constexpr-allocation.md),
[`STATIC` constants](overview/language/static-compile-time-constants.md),
[`NEW` and `DELETE`](overview/language/new-and-delete.md),
[enums](overview/language/enums.md), [flagsets](overview/language/flagsets.md),
[unions](overview/language/unions.md), [variants](overview/language/variants.md), and
[`MATCH`](overview/language/match.md).

#### Abstraction and configuration

Read about [interfaces](overview/language/interfaces-and-implementations.md),
[generics](overview/language/generics.md),
[conditional declarations](overview/language/availability-and-targets.md),
[privacy](overview/language/privacy.md), and
[build options](overview/language/build-options.md).

#### Interoperation, concurrency, and testing

Read about [serialization](overview/language/serialization.md),
[integer encodings](overview/language/integer-serialization.md),
[external procedures](overview/language/external-procedures.md),
[assembly procedures](overview/language/assembly-procedures.md),
[atomic objects](overview/language/atomics.md),
[thread-local variables](overview/language/thread-local-variables.md),
[tests](overview/language/tests.md), and
[explicit diagnostics](overview/language/diagnostics-and-failure.md).

## Current documentation boundary

The language reference covers implemented source forms supported by the live
parser and exercised compiler paths. Forward-looking proposals—such as planned
inheritance and virtual-polymorphism, or other work-in-progress are intentionally
not yet listed as current features. Low-level VMIR instruction specifications are
included in repository engineering documents rather than as part of the 
source-language reference.
