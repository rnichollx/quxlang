# Scope and Compatibility

Quxlang is a pre-release systems programming language and cross-compiler. The
compiler can produce native programs for several operating systems and CPU
architectures, and it also has a Cortado-backed JVM path, but the language and
toolchain are still under active development.

## Compatibility layers

Quxlang separates source compatibility, data interchange, and native ABI
compatibility:

- Source syntax is intended to become stable, but breaking changes remain
  possible before the first official release.
- Ordinary `STRUCT` layout is an implementation choice and may change between
  compiler versions or targets.
- `IPC_STRUCT` and `IPC_ENUM` provide explicit boundary-oriented layouts where
  a stable external representation is required.
- `EXTERN_TYPE`, `EXTERN_PROCEDURE`, and calling-convention declarations model
  interfaces to an external platform ABI.

An ordinary Quxlang type should therefore not be treated as a permanent wire or
plugin ABI merely because its current native layout happens to match another
language.

## Source bundles, not installed environments

The compilation input is a complete source bundle plus `qxcbuild.yml` and the
selected compiler version. `qxc` does not search the host's installed headers,
libraries, or package database. Runtime and library code that a program needs
belongs in the bundle's modules.

This boundary is central to reproducibility: host state may affect whether the
compiler has enough resources to finish, but it does not silently select a
different library or target configuration.

## Backend guarantees

Native compilation is the zero-overhead guarantee boundary: abstractions are
expected to preserve native efficiency. Cortado and other non-native backends
aim for language feature parity, but they do not promise native object layout
or zero-overhead representation.

Code that depends on native layout, raw addresses, assembly, or a platform ABI
must use target selection so a portable alternative can be provided where the
operation is unavailable.

## Current documentation boundary

This website documents syntax and behavior supported by the current compiler
and exercised by the repository's source bundles. Unimplemented syntax is
explicitly labeled as such. Design plans elsewhere in the repository are not
presented as completed language features.

See [Source bundles and targets](../guide/source-bundles.md),
[Availability and target selection](../reference/availability-and-targets.md),
and [Backends and layout](../reference/toolchain/backends-and-layout.md).
