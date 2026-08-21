# Quxlang

Quxlang is a systems programming language built around deterministic source
bundles, whole-program compilation, value semantics, explicit lifetime, and
cross-platform code generation.

This website documents the current compiler surface with feature-focused pages
and source examples. Quxlang remains under active development and has not yet
made a stable language release, so the current reference is descriptive rather
than a long-term compatibility promise.

## Start here

- [Getting Started](guide/index.md) explains the source-bundle model and the
  shortest path to a first program.
- [First Program](guide/first-program.md) walks through one complete source file.
- [Syntax at a Glance](syntax-examples.md) is a compact lookup page.
- [Language Feature Index](language/index.md) links every implemented language
  feature documented by the site.

## Browse the language

### Values and computation

Read about [primitive types](language/primitive-types-and-literals.md),
[arrays](language/arrays-and-construction.md),
[references and pointers](language/references-and-pointers.md),
[operators](language/arithmetic-and-comparisons.md), and
[conversions](language/conversions.md).

### Functions and reusable code

Read about [functions](language/functions-and-parameters.md), the explicit
[call argument model](language/call-arguments.md),
[overloads](language/overloads-and-enablement.md),
[templates](language/templates-and-value-parameters.md),
[variadic packs](language/variadic-packs.md), and
[lambdas](language/lambdas.md).

### Control flow and compile-time execution

Read about [conditions and loops](language/conditions-and-loops.md),
[`FOR` clauses](language/for-clauses.md),
[labels](language/labels-and-goto.md), and
[static evaluation](language/static-evaluation.md).

### Data modeling and lifetime

Read about [structs](language/structs-and-members.md),
[constructors and destructors](language/constructors-and-destructors.md),
[explicit storage lifetime](language/typed-storage-and-lifetime.md),
[compile-time allocation](language/constexpr-allocation.md),
[allocation regions](language/allocation-regions.md),
[static objects](language/static-objects-and-materialization.md),
[`NEW` and `DELETE`](language/new-and-delete.md),
[enums](language/enums.md), [flagsets](language/flagsets.md),
[unions and variants](language/unions-and-variants.md), and
[`MATCH`](language/match.md).

### Abstraction and configuration

Read about [interfaces](language/interfaces-and-implementations.md),
[generics](language/generics.md),
[conditional declarations](language/availability-and-targets.md),
[privacy](language/privacy.md), and
[build options](language/build-options.md).

### Systems and validation

Read about [serialization](language/serialization-and-stringlike.md),
[integer encodings](language/integer-serialization.md),
[external procedures](language/external-types-and-procedures.md),
[assembly procedures](language/assembly-procedures.md),
[atomic objects](language/atomics.md),
[concurrency](language/concurrency-and-per-thread-storage.md),
[tests](language/tests.md), and
[explicit diagnostics](language/diagnostics-and-failure.md).

## Toolchain and design

The [Toolchain](toolchain/index.md) section documents bundle targets, outputs,
CPU steppings, [runtime module contracts](toolchain/runtime-module-contracts.md),
backend layout, and compiler artifacts. The
[Philosophy](philosophy/overview.md) section explains the design goals behind
determinism, value semantics, safety boundaries, and control flow.

## Current documentation boundary

The language reference covers implemented source forms supported by the live
parser and exercised compiler paths. Forward-looking proposals—such as planned
inheritance and virtual-polymorphism work—are intentionally not listed as
current features. Low-level VMIR instruction specifications remain repository
engineering documents rather than part of the source-language reference.
