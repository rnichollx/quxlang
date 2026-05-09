# Project Status and TODOs

This file tracks the current implementation status and remaining high-level gaps.
Design notes may describe intended syntax or semantics that are not implemented
yet.

## Current status

The main validated execution path is still constexpr execution through VMIR2.
The static-test suite currently exercises the frontend, query graph, VMIR2
generation, and constexpr interpreter across multiple modules.

Native code generation and linking are not ready for use. There is LLVM/object
generation code in the tree, but it is incomplete and not the primary tested
path.

Last reviewed against the static-test sources on 2026-04-24. The existing
`quxlang/cmake-build-debug/quxlang_static_gtests` binary passed 81 static tests
for `Module_main_linux_arm64`.

## Implemented or mostly working

These features have current static-test coverage or direct implementation
coverage:

- [x] Classes and class fields
- [x] Constructors, destructors, copy construction, move construction, and
      default assignment
- [x] Operator overloading
- [x] Function calls, overload resolution, named arguments, and reordered named
      arguments
- [x] Function templates and `AUTO` parameter type deduction
- [x] Variadic positional packs with `PACK_SIZE`, `PACK_ARG`, and
      `PACK_ARG_TYPE`
- [x] Functions, references, and pointer/reference qualification
- [x] Instance pointers, array pointers, procedure pointers, pointer null tests,
      and pointer arithmetic
- [x] Integer operations, comparisons, casts, checked narrowing, partial
      narrowing, bitwise operators, and logical operators
- [x] If statements and while statements
- [x] Static control flow with `STATIC_IF`, `STATIC_ELSE`, and `STATIC_WHILE`
- [x] Variable declarations
- [x] Function-local `STATIC` constants and `STATIC_VAR` variables, including
      `STATIC_EVAL` mutation and `SNAPSHOT`
- [x] Assignment statements
- [x] `CONST&`, `MUT&`, `TEMP&`, and related references
- [x] Constexpr evaluation of expressions that result in `bool`
- [x] Antestatal global `STATIC` constants for primitive values, pointers, and
      procedure pointers
- [x] Constexpr storage, placement construction, destruction, allocation, and
      deallocation checks
- [x] Arrays, array indexing, array default construction, and string constants
- [x] Serialization and deserialization for primitives and simple datatype
      structs
- [x] Using multiple modules together
- [x] `INCLUDE_IF` / `ENABLE_IF`-style conditional availability
- [x] Static-test expected-failure handling for compilation failures and runtime
      assertion failures
- [x] For loops
- [x] Compound assignments
- [x] Floating point numbers
- [x] Lambda functions / closures
- [x] Defaulted function arguments
- [x] AUTO return type inference

## Remaining language features

- [ ] Atomic operations
- [ ] Exceptions and unwinding
- [ ] Polymorphism / virtual functions 
- [ ] Coroutines

## Compiler and runtime

- [ ] Non-constexpr codegen and linking
- [ ] Complete and validate the VMIR2 LLVM/object backend
- [ ] Complete runtime unwinding support
- [ ] Native runtime support and standard-library allocator integration



