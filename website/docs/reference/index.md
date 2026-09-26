# Quxlang Reference

The Reference specifies the implemented Quxlang language and toolchain. Its
scope includes accepted syntax, semantic constraints, failure behavior, target
availability, and interactions between features. The
[Reference Conventions](conventions.md) define the terminology and scope used
throughout these pages.

## Language syntax

| Topic | Contents |
| --- | --- |
| [Lexical Structure](lexical-structure.md) | Tokens, comments, identifiers, keywords, and source text |
| [Source Files and Imports](source-files-and-imports.md) | File preamble, modules, `IMPORT`, aliases, and conditional imports |
| [Namespaces](namespaces.md) | Namespace declarations, qualification, aliases, and lookup |
| [Declaration Documentation](declaration-documentation.md) | `DOC` attachment and declaration coverage |
| [Privacy](privacy.md) | Module visibility and exported aliases |
| [Target Availability](availability-and-targets.md) | `INCLUDE_IF`, target predicates, and runtime CPU queries |

## Declarations, values, and types

| Topic | Contents |
| --- | --- |
| [Variables](variables.md) | `VAR`, initialization forms, scope, storage duration, and `AUTO` |
| [Thread-Local Variables](thread-local-variables.md) | `VAR PER_THREAD`, initialization, teardown, and target support |
| [`STATIC` Constants](static-compile-time-constants.md) | Compile-time constant declarations and evaluation |
| [Primitive Types and Literals](primitive-types-and-literals.md) | Built-in scalar types and literal categories |
| [Arrays](arrays.md) | Fixed-size arrays, initialization, indexing, and element access |
| [Composites](composites.md) | Named and positional fields, transforms, reflection, and argument expansion |
| [References](references.md) | Access qualifiers, binding, conversion, and lifetime requirements |
| [Pointers](pointers.md) | Pointer classes, operations, conversions, and validity |
| [Optional and Owning Values](optional-and-owning-values.md) | `OPTIONAL`, `GENERIC`, `GENERIC_REF`, and ownership contracts |
| [Type Queries and Deduction](type-queries-and-deduction.md) | `AUTO`, `DECLTYPE`, type properties, and dynamic type identity |
| [Conversions](conversions.md) | Implicit and explicit conversion modes and their failure contracts |

## Functions and generic programming

| Topic | Contents |
| --- | --- |
| [Functions](functions-and-parameters.md) | Function declarations, parameters, receivers, and return types |
| [`RETURN` Statements](return-statements.md) | Value returns, reference returns, conditional returns, and ordering returns |
| [Call Arguments](call-arguments.md) | Named and positional groups, evaluation, forwarding, and construction |
| [Default Arguments](default-arguments.md) | Declaration context, omission, and overload participation |
| [Overload Resolution](overload-resolution.md) | Candidate construction, viability, ranking, and ambiguity |
| [Procedure Pointers and Function Values](procedure-pointers-and-function-values.md) | Callable types, binding, indirect calls, and conversions |
| [Templates](templates-and-value-parameters.md) | Type and value parameters, deduction, and instantiation |
| [Variadic Packs](variadic-packs.md) | Pack declarations, expansion, indexing, and forwarding |
| [Lambdas](lambdas.md) | Capture, callable interface, lifetime, and conversion |
| [Interfaces and Implementations](interfaces-and-implementations.md) | Interface requirements, implementation selection, and handles |
| [Generics](generics.md) | Type erasure, ownership, constraints, and dispatch |

## Expressions and statements

| Topic | Contents |
| --- | --- |
| [Operator Precedence](operator-precedence.md) | Expression grouping and associativity |
| [Assignment Operators](assignment-operators.md) | Assignment contracts and compound forms |
| [Increment and Decrement](increment-and-decrement.md) | Prefix and postfix forms and supported operands |
| [Swap Operator](swap-operator.md) | Built-in and user-defined swap behavior |
| [Move Semantics](move-semantics.md) | Temporary references, relocation, and moved-from values |
| [Arithmetic Operators](arithmetic-operators.md) | Numeric operations, checked forms, assumed forms, and overflow |
| [Comparison Operators](comparison-operators.md) | Equality, three-way comparison, derived relations, and dispatch |
| [Floating-Point Ordering](floating-point-ordering.md) | Total ordering and IEEE predicate operations |
| [Logical Operators](logical-operators.md) | Boolean operations, short-circuiting, and conditional forms |
| [Bitwise Operators](bitwise-operators.md) | Integer bit operations, shifts, and rotations |
| [User-Defined Operators](user-defined-operators.md) | Operator declarations, reflected forms, and selection |
| [Conditional Statements](conditional-statements.md) | `IF`, `ELSE`, branch conditions, and scope |
| [`LOOP WHILE` Loops](while-loops.md) | Pre-test and post-test conditional iteration |
| [`LOOP` Statements](loop-statements.md) | Range, iterator, collection, and unconditional loop forms |
| [Labels and `GOTO`](labels-and-goto.md) | Labels, jumps, scope exits, and restrictions |
| [Exception Handling](exceptions.md) | Throwing, matching, cleanup, rethrow, handles, and `NOEXCEPT` |
| [Failure Statements](diagnostics-and-failure.md) | Assertions, panic, compilation errors, and unimplemented paths |
| [Compilation Policies](compilation-policies.md) | Per-output assertion, bounds, overflow, and unimplemented behavior |

## Structures, lifetime, and alternatives

| Topic | Contents |
| --- | --- |
| [Structures](structs-and-members.md) | Members, receivers, nested declarations, and generated operations |
| [Public Field Reflection](public-field-reflection.md) | Public-field enumeration and access queries |
| [Inheritance](inheritance.md) | Base subobjects, polymorphism, casts, construction, and destruction |
| [Constructors and Destructors](constructors-and-destructors.md) | Object initialization, teardown, and generated functions |
| [Object Storage and Lifetime](typed-storage-and-lifetime.md) | Raw storage, `PLACE`, `DESTROY`, and active objects |
| [Compile-Time Allocation](constexpr-allocation.md) | Allocation regions and constant-evaluation storage |
| [`NEW` and `DELETE`](new-and-delete.md) | Typed allocation, construction, destruction, and deallocation |
| [Enums](enums.md) | Nominal enumerations, values, conversions, and generated operations |
| [Flagsets](flagsets.md) | Bit flag declarations, widths, operations, and conversion |
| [Unions](unions.md) | Untagged alternatives, active members, lifetime, and generated operations |
| [Variants](variants.md) | Tagged alternatives, construction, access, and state |
| [`VISIT`](visit.md) | Variant visitation, callable requirements, and return types |
| [`MATCH`](match.md) | Pattern arms, binding, coverage, and control flow |

## Evaluation, interoperation, and execution

| Topic | Contents |
| --- | --- |
| [Compile-Time Evaluation](compile-time-evaluation.md) | Constant execution, state, restrictions, and diagnostics |
| [Runtime Selection](runtime-selection.md) | `RUNTIME CONSTEXPR` and `RUNTIME NATIVE` selection |
| [Serialization](serialization.md) | Generated and custom serialization contracts |
| [Stringlike Types](stringlike-types.md) | Character sequences and stringlike interfaces |
| [Variable-Length Integer Serialization](integer-serialization.md) | Integer encodings and size behavior |
| [External Types](external-types.md) | External layout declarations and representation constraints |
| [External Memory](external-memory.md) | `IBC` access and runtime address operations |
| [External Procedures](external-procedures.md) | Native symbols, libraries, calling conventions, and target formats |
| [Assembly Procedures](assembly-procedures.md) | Architecture-specific callable bodies and symbolic operands |
| [Atomics](atomics.md) | Atomic operations, ordering, waiting, and notification |
| [Tests](tests.md) | Static, unit, and dual tests and their execution modes |

## Toolchain and output

| Topic | Contents |
| --- | --- |
| [Toolchain](toolchain.md) | Compiler and build-tool roles |
| [`qxcbuild.yml`](qxcbuild-file.md) | Bundle, target, output, and library configuration |
| [Build Options](build-options.md) | Build options available to source code |
| [CPU Capabilities and Steppings](cpu-capabilities-and-steppings.md) | Capability names, detection, and multiversion selection |
| [Program Startup and Runtime Hooks](program-startup-and-runtime-hooks.md) | Entry procedures, stepping arrays, and startup contracts |
| [Runtime Module Contracts](runtime-module-contracts.md) | Required runtime declarations and target-specific obligations |
| [Backends and Layout](backends-and-layout.md) | Backend boundaries and native versus layoutless targets |
| [Compiler Output](compiler-output.md) | Emitted artifacts and output-specific behavior |

Forward-looking proposals and internal VMIR formats are outside this Reference.
