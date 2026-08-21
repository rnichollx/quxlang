# Syntax at a Glance

This page is the compact map of current Quxlang syntax. Each section links to a
feature page with complete examples, constraints, and related concepts.

## Complete source shape

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;

::add_numbers FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

::main FUNCTION(): I32
{
  VAR result I32 := add_numbers(% [2, 3]);
  RETURN result;
}
```

Start with [the first-program walkthrough](guide/first-program.md) or browse the
complete [language feature index](language/index.md).

## Declaration forms

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Identifier | `source_value2` | [Lexical structure](language/lexical-structure.md) |
| Module declaration | `::name ...` | [Names and scopes](language/names-namespaces-and-scopes.md) |
| Instance member | `.name ...` | [Structs and members](language/structs-and-members.md) |
| Mutable object | `VAR value I32 := 1;` | [Variables](language/variables-and-storage.md) |
| Static object | `STATIC value I32 := 1;` | [Static objects](language/static-objects-and-materialization.md) |
| Per-thread object | `::value PER_THREAD VAR I32;` | [Concurrency](language/concurrency-and-per-thread-storage.md) |
| Function | `FUNCTION(@value I32): I32` | [Functions](language/functions-and-parameters.md) |
| Template | `TEMPLATE(TYPE AUTO(t))` | [Templates](language/templates-and-value-parameters.md) |
| Struct | `STRUCT { ... }` | [Structs](language/structs-and-members.md) |
| Enum | `ENUM [first DEFAULT, second]` | [Enums](language/enums.md) |
| Flagset | `FLAGSET BITS(8) [read, write]` | [Flagsets](language/flagsets.md) |
| Union | `INLINE_UNION { .value OPTION I32; }` | [Unions and variants](language/unions-and-variants.md) |
| Variant | `INLINE_VARIANT [I32 DEFAULT, VOID]` | [Unions and variants](language/unions-and-variants.md) |
| Interface | `INTERFACE { ... }` | [Interfaces](language/interfaces-and-implementations.md) |
| Owning erasure | `GENERIC { ... }` | [Generics](language/generics.md) |
| Build option | `OPTION BOOL DEFAULT(FALSE);` | [Build options](language/build-options.md) |

## Types and values

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Integer | `I32`, `U128`, `U24` | [Primitive types](language/primitive-types-and-literals.md) |
| Float | `F32`, `F64`, `F16E5` | [Primitive types](language/primitive-types-and-literals.md) |
| Array | `[4]I32` | [Arrays](language/arrays-and-construction.md) |
| Reference | `MUT& I32`, `CONST& I32` | [References and pointers](language/references-and-pointers.md) |
| Instance pointer | `MUT->I32` | [References and pointers](language/references-and-pointers.md) |
| Array pointer | `MUT=>>I32` | [References and pointers](language/references-and-pointers.md) |
| Managed reference | `~>java_object` | [External types](language/external-types-and-procedures.md) |
| Procedure | `PROCEDURE(I32: BOOL)` | [Procedure pointers](language/procedure-pointers-and-function-values.md) |
| Typed storage | `TYPED_STORAGE(point)` | [Typed storage](language/typed-storage-and-lifetime.md) |
| Type query | `TYPEOF(value)`, `SIZEOF(I32)` | [Type queries](language/type-queries-and-deduction.md) |

## Calls and construction

```quxlang
named(@value 3, @scale 2);
positional(% [3, 2]);
interleaved(% [first], @named second, % [third]);
single_arg(value); // shorthand for @ARG

VAR defaulted point;
VAR copied point := other;
VAR named point :(@x 3, @y 4);
VAR positional point :[3, 4];
```

Read [Call arguments](language/call-arguments.md),
[Arrays and construction](language/arrays-and-construction.md), and
[Constructors and destructors](language/constructors-and-destructors.md).

## Control flow

[Operator precedence](language/operator-precedence.md) records how expression
forms group when parentheses are omitted.

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Condition | `IF (...) { ... } ELSE { ... }` | [Conditions and loops](language/conditions-and-loops.md) |
| Negative condition | `UNLESS (...) { ... }` | [Conditions and loops](language/conditions-and-loops.md) |
| Loop | `WHILE (...) { ... }` | [Conditions and loops](language/conditions-and-loops.md) |
| Clause loop | `FOR VALUE(i) FROM(0) UNTIL(4) LOOP { ... };` | [`FOR` clauses](language/for-clauses.md) |
| Compile-time branch | `STATIC_IF(...) { ... } STATIC_ELSE { ... }` | [Static evaluation](language/static-evaluation.md) |
| Runtime-mode branch | `RUNTIME NATIVE { ... } ELSE { ... }` | [Static evaluation](language/static-evaluation.md) |
| Fusion dispatch | `MATCH value { TYPE I32 { ... } }` | [`MATCH`](language/match.md) |
| Labeled exit | `BREAK :outer;` | [Labels and `GOTO`](language/labels-and-goto.md) |

## Lifetime and systems forms

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Placement | `PLACE AT(storage) point:[1, 2]` | [Typed storage](language/typed-storage-and-lifetime.md) |
| Lifetime access | `PUN storage AS point` | [Typed storage](language/typed-storage-and-lifetime.md) |
| Destruction | `DESTROY AT(storage) point;` | [Typed storage](language/typed-storage-and-lifetime.md) |
| Compile-time allocation | `CONSTEXPR_ALLOC#I32()` | [Compile-time allocation](language/constexpr-allocation.md) |
| Allocation region | `BEGIN_ALLOC_REGION address TO ->TYPED_STORAGE(I32)` | [Allocation regions](language/allocation-regions.md) |
| Existing object discovery | `ADDRESS_LAUNDER_DISCOVER_EXISTING address TO MUT->I32` | [Addresses and provenance](language/address-provenance.md) |
| Allocation | `NEW point :(@x 1, @y 2)` | [`NEW` and `DELETE`](language/new-and-delete.md) |
| Deallocation | `DELETE pointer;` | [`NEW` and `DELETE`](language/new-and-delete.md) |
| Conditional declaration | `INCLUDE_IF(OS_LINUX)` | [Availability](language/availability-and-targets.md) |
| Privacy | `PRIVATE(MODULE) ::name ...` | [Privacy](language/privacy.md) |
| Native symbol | `EXTERN_PROCEDURE[...] CALLABLE(...)` | [External procedures](language/external-types-and-procedures.md) |
| Assembly | `ASM_PROCEDURE X64 CALLABLE(...) { ... }` | [Assembly procedures](language/assembly-procedures.md) |
| Atomic object | `ATOMIC#I32` | [Atomic objects](language/atomics.md) |
| Integer encoding | `SERIALIZE_LEB128(...)` | [Integer serialization](language/integer-serialization.md) |

## Validation forms

```quxlang
::static_case STATIC_TEST { ASSERT(TRUE); }
::runtime_case UNIT_TEST { ASSERT(TRUE); }
::both_cases DUAL_TEST { ASSERT(TRUE); }

PANIC "unrecoverable state";
COMPILATION_ERROR "unsupported configuration";
UNIMPLEMENTED;
```

Read [Tests](language/tests.md) and
[Diagnostics and explicit failure](language/diagnostics-and-failure.md).
