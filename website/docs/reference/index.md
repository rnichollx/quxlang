# Quxlang Reference

The Reference division specifies the implemented Quxlang language and
toolchain. Each language feature has a direct page under `reference/`; these
links do not route through Overview pages or unrelated sections.

Read [Reference Conventions](conventions.md) for the terminology, support
boundary, and target distinctions used by the technical pages.

## Source and declarations

- [Lexical Structure](lexical-structure.md)
- [Source Files and Imports](source-files-and-imports.md)
- [Namespaces](namespaces.md)
- [Declaration Documentation](declaration-documentation.md)
- [Privacy](privacy.md)
- [Target Availability](availability-and-targets.md)
- [Build Options](build-options.md)

## Values and types

- [Variables](variables.md)
- [Thread-Local Variables](thread-local-variables.md)
- [`STATIC` Compile-Time Constants](static-compile-time-constants.md)
- [Primitive Types and Literals](primitive-types-and-literals.md)
- [Arrays](arrays.md)
- [Composites](composites.md)
- [References](references.md)
- [Pointers](pointers.md)
- [Type Queries and Deduction](type-queries-and-deduction.md)
- [Conversions](conversions.md)

## Functions and generic programming

- [Functions](functions-and-parameters.md)
- [`RETURN` Statements](return-statements.md)
- [Call Arguments](call-arguments.md)
- [Default Arguments](default-arguments.md)
- [Overload Resolution](overload-resolution.md)
- [Procedure Pointers and Function Values](procedure-pointers-and-function-values.md)
- [Templates](templates-and-value-parameters.md)
- [Variadic Packs](variadic-packs.md)
- [Lambdas](lambdas.md)
- [Interfaces and Implementations](interfaces-and-implementations.md)
- [Generics](generics.md)

## Expressions and operators

- [Operator Precedence](operator-precedence.md)
- [Assignment Operators](assignment-operators.md)
- [Increment and Decrement](increment-and-decrement.md)
- [Swap Operator](swap-operator.md)
- [Move Semantics](move-semantics.md)
- [Arithmetic Operators](arithmetic-operators.md)
- [Comparison Operators](comparison-operators.md)
- [Floating-Point Ordering](floating-point-ordering.md)
- [Logical Operators](logical-operators.md)
- [Bitwise Operators](bitwise-operators.md)
- [User-Defined Operators](user-defined-operators.md)

## Control flow and generation

- [Conditional Statements](conditional-statements.md)
- [`WHILE` Loops](while-loops.md)
- [`LOOP` Statements](loop-statements.md)
- [Labels and `GOTO`](labels-and-goto.md)
- [Compile-Time Evaluation](compile-time-evaluation.md)
- [Runtime Selection](runtime-selection.md)
- [`MATCH`](match.md)
- [`VISIT`](visit.md)

## Structures and lifetime

- [Structures](structs-and-members.md)
- [Public Field Reflection](public-field-reflection.md)
- [Inheritance](inheritance.md)
- [Constructors and Destructors](constructors-and-destructors.md)
- [Object Storage and Lifetime](typed-storage-and-lifetime.md)
- [Compile-Time Allocation](constexpr-allocation.md)
- [`NEW` and `DELETE`](new-and-delete.md)
- [Enums](enums.md)
- [Flagsets](flagsets.md)
- [Unions](unions.md)
- [Variants](variants.md)

## Interoperation, concurrency, and testing

- [Serialization](serialization.md)
- [Stringlike Types](stringlike-types.md)
- [Variable-Length Integer Serialization](integer-serialization.md)
- [External Types](external-types.md)
- [External Procedures](external-procedures.md)
- [Assembly Procedures](assembly-procedures.md)
- [Atomics](atomics.md)
- [Tests](tests.md)
- [Failure Statements](diagnostics-and-failure.md)

## Toolchain

- [Toolchain Reference](toolchain/index.md)
- [The `qxcbuild.yml` File](toolchain/qxcbuild-file.md)
- [CPU Capabilities and Steppings](toolchain/cpu-capabilities-and-steppings.md)
- [Program Startup and Runtime Hooks](toolchain/program-startup-and-runtime-hooks.md)
- [Runtime Module Contracts](toolchain/runtime-module-contracts.md)
- [Backends and Layout](toolchain/backends-and-layout.md)
- [Compiler Output](toolchain/compiler-output.md)

The Reference documents supported source forms and exercised compiler paths.
Forward-looking proposals and VMIR engineering formats are not presented as
current source-language features.
