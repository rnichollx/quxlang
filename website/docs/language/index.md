# Language Feature Index

These pages describe the current source language. Each page focuses on one
feature, gives valid syntax, and records the constraints visible to a Quxlang
programmer. Forward-looking proposals are not presented as implemented syntax.

## Fundamentals

- [Lexical structure](lexical-structure.md)
- [Source files and imports](source-files-and-imports.md)
- [Names, namespaces, and scopes](names-namespaces-and-scopes.md)
- [Declaration documentation](declaration-documentation.md)
- [Variables and storage duration](variables-and-storage.md)
- [Static objects and materialization](static-objects-and-materialization.md)
- [Primitive types and literals](primitive-types-and-literals.md)
- [Arrays and construction](arrays-and-construction.md)
- [References and pointers](references-and-pointers.md)
- [Type queries and deduction](type-queries-and-deduction.md)

## Functions and generic code

- [Functions and parameters](functions-and-parameters.md)
- [Call arguments](call-arguments.md)
- [Defaults, overloads, and enablement](overloads-and-enablement.md)
- [Procedure pointers and function values](procedure-pointers-and-function-values.md)
- [Templates and value parameters](templates-and-value-parameters.md)
- [Variadic packs](variadic-packs.md)
- [Lambdas](lambdas.md)

## Expressions and control flow

- [Operator precedence](operator-precedence.md)
- [Assignment, movement, and swap](assignment-movement-and-swap.md)
- [Arithmetic and comparisons](arithmetic-and-comparisons.md)
- [Floating-point ordering](floating-point-ordering.md)
- [Logical and booliation operators](logical-and-booliation.md)
- [Bitwise operators and `BIT`](bitwise-operators.md)
- [Conversions](conversions.md)
- [Conditions and loops](conditions-and-loops.md)
- [`FOR` clauses](for-clauses.md)
- [Labels and `GOTO`](labels-and-goto.md)
- [Static evaluation and runtime selection](static-evaluation.md)

## Data types and lifetime

- [Structs and members](structs-and-members.md)
- [Constructors and destructors](constructors-and-destructors.md)
- [User-defined operators](user-defined-operators.md)
- [Typed storage and explicit lifetime](typed-storage-and-lifetime.md)
- [Compile-time allocation](constexpr-allocation.md)
- [Allocation regions](allocation-regions.md)
- [Addresses and provenance](address-provenance.md)
- [`NEW` and `DELETE`](new-and-delete.md)
- [Enums](enums.md)
- [Flagsets](flagsets.md)
- [Unions and variants](unions-and-variants.md)
- [`MATCH`](match.md)

## Abstraction and declaration control

- [Interfaces and implementations](interfaces-and-implementations.md)
- [Owning generics and generic references](generics.md)
- [`INCLUDE_IF` and target selection](availability-and-targets.md)
- [Privacy](privacy.md)
- [Build options](build-options.md)

## Systems integration and validation

- [Serialization and stringlike types](serialization-and-stringlike.md)
- [Variable-length integer serialization](integer-serialization.md)
- [External types and procedures](external-types-and-procedures.md)
- [Assembly procedures](assembly-procedures.md)
- [Atomic objects](atomics.md)
- [Concurrency and per-thread storage](concurrency-and-per-thread-storage.md)
- [Tests](tests.md)
- [Diagnostics and explicit failure](diagnostics-and-failure.md)
