# Syntax at a Glance

This page is the compact map of current Quxlang syntax. Each section links to a
feature page with complete examples, constraints, and related concepts.

## Complete source shape

```quxlang
LANGUAGE QUXLANG EN 0.0;

IMPORT std;

::clamp FUNCTION(@value I32, @minimum I32, @maximum I32): I32
{
  IF (value < minimum)
  {
    RETURN minimum;
  }
  IF (value > maximum)
  {
    RETURN maximum;
  }
  RETURN value;
}

::main FUNCTION(): I32
{
  VAR result I32 := clamp(@value 120, @minimum 0, @maximum 100);
  ASSERT(result == 100);
  RETURN 0;
}
```

Start with [the first-program walkthrough](guide/first-program.md) or browse the
complete [language feature index](reference/index.md).

## Declaration forms

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Identifier | `source_value2` | [Lexical structure](reference/lexical-structure.md) |
| Module declaration | `::name ...` | [Names and scopes](reference/namespaces.md) |
| Instance member | `.name ...` | [Structs and members](reference/structs-and-members.md) |
| Mutable object | `VAR value I32 := 1;` | [Variables](reference/variables.md) |
| Static object | `STATIC value I32 := 1;` | [Static objects](reference/static-compile-time-constants.md) |
| Per-thread object | `::value PER_THREAD VAR I32;` | [Thread-Local Variables](reference/thread-local-variables.md) |
| Function | `FUNCTION(@value I32): I32` | [Functions](reference/functions-and-parameters.md) |
| Template | `TEMPLATE(@T TYPE AUTO)` | [Templates](reference/templates-and-value-parameters.md) |
| Struct | `STRUCT { ... }` | [Structs](reference/structs-and-members.md) |
| Base subobject | `.base_part BASE base_type;` | [Inheritance](reference/inheritance.md) |
| Virtual function | `.read FUNCTION() CONST VIRTUAL: I32` | [Inheritance](reference/inheritance.md) |
| Enum | `ENUM [first DEFAULT, second]` | [Enums](reference/enums.md) |
| Flagset | `FLAGSET BITS(8) [read, write]` | [Flagsets](reference/flagsets.md) |
| Union | `INLINE_UNION { .value OPTION I32; }` | [Unions](reference/unions.md) |
| Variant | `INLINE_VARIANT [I32 DEFAULT, VOID]` | [Variants](reference/variants.md) |
| Interface | `INTERFACE { ... }` | [Interfaces](reference/interfaces-and-implementations.md) |
| Owning erasure | `GENERIC { ... }` | [Generics](reference/generics.md) |
| Build option | `OPTION BOOL DEFAULT(FALSE);` | [Build options](reference/build-options.md) |

## Types and values

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Integer | `I32`, `U128`, `U24` | [Primitive types](reference/primitive-types-and-literals.md) |
| Float | `F32`, `F64`, `F16E5` | [Primitive types](reference/primitive-types-and-literals.md) |
| Array | `[4]I32` | [Arrays](reference/arrays.md) |
| Reference | `MUT& I32`, `CONST& I32` | [References](reference/references.md) |
| Instance pointer | `MUT->I32` | [Pointers](overview/pointers.md) |
| Array pointer | `MUT=>>I32` | [Pointers](overview/pointers.md) |
| GC pointer | `~>java_object` | [External types](overview/external-types.md) |
| Procedure | `PROCEDURE(@value I32: BOOL)` | [Procedure pointers](reference/procedure-pointers-and-function-values.md) |
| Typed storage | `TYPED_STORAGE(point)` | [Typed storage](reference/typed-storage-and-lifetime.md) |
| Type query | `TYPEOF(value)`, `SIZEOF(I32)` | [Type queries](reference/type-queries-and-deduction.md) |

## Calls and construction

```quxlang
configure_window(@width 1280, @height 720);
single_arg(value); // shorthand for @ARG

// Positional arguments are available when position is part of the API.
vector2(% [3, 2]);
log_message(% [message], @severity warning, % [context]);

VAR defaulted point;
VAR copied point := other;
VAR named point :(@x 3, @y 4);

// Positional construction is also available.
VAR positional point :[3, 4];
```

Read [Call arguments](reference/call-arguments.md),
[Arrays](reference/arrays.md), and
[Constructors and destructors](reference/constructors-and-destructors.md).

## Control flow

[Operator precedence](reference/operator-precedence.md) records how expression
forms group when parentheses are omitted.

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Condition | `IF (...) { ... } ELSE { ... }` | [Conditional statements](reference/conditional-statements.md) |
| Negative condition | `UNLESS (...) { ... }` | [Conditional statements](reference/conditional-statements.md) |
| Loop | `WHILE (...) { ... }` | [`WHILE` loops](reference/while-loops.md) |
| Clause loop | `FOR VALUE(i) FROM(0) UNTIL(4) LOOP { ... };` | [`FOR` clauses](reference/for-loops.md) |
| Compile-time branch | `STATIC_IF(...) { ... } STATIC_ELSE { ... }` | [Compile-Time Evaluation](reference/compile-time-evaluation.md) |
| Runtime-mode branch | `RUNTIME NATIVE { ... } ELSE { ... }` | [Runtime Selection](reference/runtime-selection.md) |
| Fusion dispatch | `MATCH value { TYPE I32 { ... } }` | [`MATCH`](reference/match.md) |
| Variant specialization | `VISIT value { consume(@value value); }` | [`VISIT`](reference/visit.md) |
| Labeled exit | `BREAK :outer;` | [Labels and `GOTO`](reference/labels-and-goto.md) |

## Lifetime and systems forms

| Feature | Representative syntax | Reference |
| --- | --- | --- |
| Placement | `PLACE AT(storage) point:[1, 2]` | [Typed storage](reference/typed-storage-and-lifetime.md) |
| Lifetime access | `PUN storage AS point` | [Typed storage](reference/typed-storage-and-lifetime.md) |
| Destruction | `DESTROY AT(storage) point;` | [Typed storage](reference/typed-storage-and-lifetime.md) |
| Compile-time allocation | `CONSTEXPR_ALLOC#I32()` | [Compile-time allocation](reference/constexpr-allocation.md) |
| Allocation | `NEW point :(@x 1, @y 2)` | [`NEW` and `DELETE`](reference/new-and-delete.md) |
| Deallocation | `DELETE pointer;` | [`NEW` and `DELETE`](reference/new-and-delete.md) |
| Checked hierarchy cast | `pointer AS DYNAMIC MUT->derived` | [Inheritance](reference/inheritance.md) |
| Conditional declaration | `INCLUDE_IF(OS_LINUX)` | [Availability](reference/availability-and-targets.md) |
| Privacy | `PRIVATE(MODULE) ::name ...` | [Privacy](reference/privacy.md) |
| Native symbol | `EXTERN_PROCEDURE[...] CALLABLE(...)` | [External procedures](reference/external-procedures.md) |
| Assembly | `ASM_PROCEDURE X64 CALLABLE(...) { ... }` | [Assembly procedures](reference/assembly-procedures.md) |
| Atomic object | `ATOMIC#I32` | [Atomic objects](reference/atomics.md) |
| Integer encoding | `SERIALIZE_LEB128(...)` | [Integer serialization](reference/integer-serialization.md) |

## Validation forms

```quxlang
::static_case STATIC_TEST { ASSERT(TRUE); }
::runtime_case UNIT_TEST { ASSERT(TRUE); }
::both_cases DUAL_TEST { ASSERT(TRUE); }

PANIC "unrecoverable state";
COMPILATION_ERROR "unsupported configuration";
UNIMPLEMENTED;
```

Read [Tests](reference/tests.md) and
[Diagnostics and explicit failure](reference/diagnostics-and-failure.md).
