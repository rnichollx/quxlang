# Project Status and TODOs

This file tracks the current implementation status and remaining high-level gaps.
Design notes may describe intended syntax or semantics that are not implemented
yet.

## Current status

Quxlang can compile and link native Linux executables, including current
testmodule unit tests. The compiler is still pre-release and not ready for
production use.

## Language and standard-library status

Checked items have current static-test, dual-test, unit-test, or direct
implementation coverage. Unchecked items are remaining high-level work.

### Object model and calls

- [x] Struct declarations and fields
- [x] Constructors, destructors, copy construction, move construction, and
      default assignment
- [x] Operator overloading
- [x] Function calls, overload resolution, named arguments, and reordered named
      arguments
- [x] Functions, references, and pointer/reference qualification
- [x] Instance pointers, array pointers, procedure pointers, pointer null tests,
      and pointer arithmetic
- [x] Interfaces and interface dispatch
- [x] Function and bound-functum bindings
- [ ] Exceptions and unwinding
- [ ] Polymorphism / virtual functions
- [ ] C++ style inheritance
- [ ] LLVM style inheritance
- [ ] Generics

### Types and expressions

- [x] Integer operations, comparisons, casts, checked narrowing, partial
      narrowing, bitwise operators, and logical operators
- [x] `CONST&`, `MUT&`, `TEMP&`, and related references
- [x] Arrays, array indexing, array default construction, and string constants
- [x] Serialization and deserialization for primitives and simple datatype
      structs
- [x] Floating point numbers
- [x] Atomic operations
- [x] Enums and IPC enums
- [x] Flagsets
- [x] `TYPEOF`, `DECLTYPE`, `FORWARD`, `SAME_TYPES`, `SIZEOF`, and `BITS`
      type expressions
- [ ] Region casting and dynamic allocation-region expressions

### Statements and control flow

- [x] Variable declarations
- [x] Assignment statements
- [x] Compound assignments
- [x] `ASSERT`, `UNIMPLEMENTED`, and `COMPILATION_ERROR` statements
- [x] Goto statements
- [x] If statements and while statements
- [x] For loops
- [x] `RUNTIME` statement branching
- [x] Static control flow with `STATIC_IF`, `STATIC_ELSE`, and `STATIC_WHILE`
- [x] Lambda functions / closures
- [x] Defaulted function arguments
- [x] AUTO return type inference
- [ ] Coroutines
- [ ] Range-based for and container iteration
- [ ] Scoped with statements

### Compile-time evaluation and metaprogramming

- [x] Constexpr evaluation of expressions that result in `bool`
- [x] Function-local `STATIC` constants and `STATIC_VAR` variables, including
      `STATIC_EVAL` mutation and `SNAPSHOT`
- [x] Antestatal global `STATIC` constants for primitive values, pointers, and
      procedure pointers
- [x] Constexpr storage, placement construction, destruction, allocation, and
      deallocation checks
- [x] Function templates and `AUTO` parameter type deduction
- [x] Template value parameters in dependent types and return types
- [x] Variadic positional packs with `PACK_SIZE`, `PACK_ARG`, and
      `PACK_ARG_TYPE`
- [x] `OPTION` declarations and compile-time option lookup
- [x] `INCLUDE_IF` / `ENABLE_IF`-style conditional availability
- [x] Architecture, OS, binary-format, and environment predicates
- [ ] Reflection
- [ ] Registries

### Modules, tests, and native execution

- [x] Using multiple modules together
- [x] Static-test expected-failure handling for compilation failures and runtime
      assertion failures
- [x] Dual static/unit tests with `DUAL_TEST`
- [x] Architecture-selected `ASM_PROCEDURE` declarations
- [x] ASM `OBJECT_REF` operands and custom native entrypoints with
      `PROGRAM_START`, `UNIT_TESTING_PROGRAM_START`, and `MAIN_FUNCTION`
- [x] Linux ELF executable generation and linking
- [x] Linux native unit-test execution through the testmodule suite
- [x] Native syscall tests for Linux targets
- [x] Native allocator entry points covered by current memory unit tests
- [x] `PER_THREAD` global storage and thread-local initguard lowering

## Compiler and runtime

- [x] Linux ELF binaries
- [x] ELF metadata preservation, symbol table generation, and readable symbol
      display names
- [x] Zero-initialized global storage and static serialoid link support
- [x] Static extern procedure declarations and linksymbol metadata
- [ ] Dynamic extern import support (partially added, not tested)
- [ ] Complete runtime unwinding support
- [ ] Complete native runtime support and standard-library integration
- [ ] Proper debugging support
- [ ] Configurable CPU/subarchitecture and target-feature selection
- [ ] PE/COFF binaries (untested)
- [ ] Mach-O binaries for MacOS
