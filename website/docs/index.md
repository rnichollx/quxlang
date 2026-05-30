# Quxlang

Quxlang is a systems programming language related to C++, built around
deterministic builds, whole-program static compilation, first-class modules, and
less ambiguous syntax.

## Direction

Quxlang aims to improve C++ for systems programming without keeping C++'s
historical build model. Source is compiled as a source bundle, modules are part
of the language model, and compiler output is intended to be deterministic and
reproducible.

## Main Points

- deterministic, reproducible builds
- whole-program static compilation
- first-class modules, with no header files
- declaration-order-independent compilation
- multi-threaded compilation rather than multi-process compilation
- unambiguous primitive type names such as `I32`, not contextual names such as
  `short` or `long`
- left-to-right type symbols and suffix-oriented syntax where that reduces
  ambiguity
- uppercase keywords, so future keywords can be added without breaking ordinary
  lowercase identifiers

## Current project state

Quxlang is still under active development. These pages describe the intended
language design and documented semantics.

## Reading path

- Start with [Overview](philosophy/overview.md) for the overall language
  direction.
- Read the Philosophy topic pages for the main build, syntax, and compatibility
  decisions.
- Use the tutorial for example-driven reading and the reference for rule-driven
  reading.
- Use the advanced section for ABI, VMIR2, and lowering details.
