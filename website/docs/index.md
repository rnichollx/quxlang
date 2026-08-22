# Quxlang

Quxlang is a systems programming language built around deterministic source
bundles, whole-program compilation, value semantics, explicit lifetime, and
cross-platform code generation.

This website documents the current compiler surface with feature-focused pages
and source examples. Quxlang remains under active development and has not yet
made a stable language release, so the current reference is descriptive rather
than a long-term compatibility promise.

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

## Reference

The [Reference Index](reference/index.md) gives the comprehensive technical
contract for every implemented language feature. The
[Toolchain Reference](reference/toolchain/index.md) documents source-bundle targets,
compiler outputs, CPU steppings, runtime module contracts, and backend behavior.

### Browse the language

#### Values and computation

Read about [primitive types](reference/primitive-types-and-literals.md),
[arrays](reference/arrays.md),
[references](reference/references.md), [pointers](reference/pointers.md),
[arithmetic operators](reference/arithmetic-operators.md),
[comparison operators](reference/comparison-operators.md), and
[conversions](reference/conversions.md).

#### Functions and reusable code

Read about [functions](reference/functions-and-parameters.md), the explicit
[call argument model](reference/call-arguments.md),
[overloads](reference/overload-resolution.md),
[templates](reference/templates-and-value-parameters.md),
[variadic packs](reference/variadic-packs.md), and
[lambdas](reference/lambdas.md).

#### Control flow and compile-time execution

Read about [conditional statements](reference/conditional-statements.md),
[`WHILE` loops](reference/while-loops.md),
[`FOR` clauses](reference/for-loops.md),
[labels](reference/labels-and-goto.md), and
[static evaluation](reference/compile-time-evaluation.md).

#### Data modeling and lifetime

Read about [structs](reference/structs-and-members.md),
[constructors and destructors](reference/constructors-and-destructors.md),
[explicit storage lifetime](reference/typed-storage-and-lifetime.md),
[compile-time allocation](reference/constexpr-allocation.md),
[`STATIC` constants](reference/static-compile-time-constants.md),
[`NEW` and `DELETE`](reference/new-and-delete.md),
[enums](reference/enums.md), [flagsets](reference/flagsets.md),
[unions](reference/unions.md), [variants](reference/variants.md), and
[`MATCH`](reference/match.md).

#### Abstraction and configuration

Read about [interfaces](reference/interfaces-and-implementations.md),
[generics](reference/generics.md),
[conditional declarations](reference/availability-and-targets.md),
[privacy](reference/privacy.md), and
[build options](reference/build-options.md).

#### Interoperation, concurrency, and testing

Read about [serialization](reference/serialization.md),
[integer encodings](reference/integer-serialization.md),
[external procedures](reference/external-procedures.md),
[assembly procedures](reference/assembly-procedures.md),
[atomic objects](reference/atomics.md),
[concurrency](reference/thread-local-variables.md),
[tests](reference/tests.md), and
[explicit diagnostics](reference/diagnostics-and-failure.md).

## Current documentation boundary

The language reference covers implemented source forms supported by the live
parser and exercised compiler paths. Forward-looking proposals—such as planned
inheritance and virtual-polymorphism work—are intentionally not listed as
current features. Low-level VMIR instruction specifications remain repository
engineering documents rather than part of the source-language reference.
