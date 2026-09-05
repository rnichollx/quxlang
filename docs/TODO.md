# Project Status and TODOs

This file tracks the current implementation status and remaining high-level gaps.
Design notes may describe intended syntax or semantics that are not implemented
yet.

## Current status

Quxlang can compile and link native Linux ELF, Windows PE/COFF, and macOS Mach-O
executables, and generate JVM JARs through the Cortado backend. Target support
and validation vary as noted below. The compiler is still pre-release and not
ready for production use.

## Language and standard-library status

Checked items have current static-test, dual-test, unit-test, or direct
implementation coverage, not necessarily support on every target. Unchecked
items are remaining high-level work.

### Object model and calls

- [x] Struct declarations and fields
- [x] Constructors, destructors, copy construction, move construction, and
      default assignment
- [x] Operator overloading
- [x] Function calls, overload resolution, explicit `% [...]` positional groups,
      single-argument `@ARG` shorthand, named arguments, and reordered named arguments
- [x] Functions, references, and pointer/reference qualification
- [x] Instance pointers, array pointers, procedure pointers, pointer null tests,
      and pointer arithmetic
- [x] Interfaces and interface dispatch
- [x] Function and bound-functum bindings
- [x] Polymorphism / virtual functions, overrides, abstract slots, and virtual
      destructors on native targets
- [x] C++ style inheritance with ordinary, multiple, and virtual bases,
      inherited member lookup, and base conversions on native targets
- [x] Dynamic pointer downcasts and cross-casts with `AS DYNAMIC` on native
      targets
- [x] Generics with owning `GENERIC`, non-owning `GENERIC_REF`, and transitive
      `IMPLEMENTS` contracts
- [x] `ROOTED` structs with stable object addresses and copy/move restrictions
- [ ] Exceptions and unwinding
- [ ] LLVM style inheritance
- [ ] Inheritance and polymorphic dispatch on the JVM backend

### Types and expressions

- [x] Integer operations, comparisons, casts, checked narrowing, partial
      narrowing, bitwise operators, `BIT n` literals, and logical operators
- [x] `CONST&`, `MUT&`, `TEMP&`, and related references
- [x] Arrays, array indexing, array default construction, and string constants
- [x] Serialization and deserialization for primitives and simple datatype
      structs
- [x] Floating point numbers
- [x] Atomic operations
- [x] Enums and IBC enums
- [x] Flagsets
- [x] `UNION`, `VARIANT`, `INLINE_UNION`, and `INLINE_VARIANT`, including
      alternative tests and `UNWRAP`
- [x] Three-way comparisons with `<=>` and `ORDER`
- [x] `TYPE_INDEX` and `TYPE_INDEX_OF` type identity and ordering
- [x] `NULL_TYPE` and null conversions
- [x] `TYPEOF`, `DECLTYPE`, `FORWARD`, `SAME_TYPES`, `SIZEOF`, `ALIGNOF`, and `BITS`
      type expressions
- [x] `NEW` and `DELETE`, including constructor selection and destruction
- [x] Basic address/storage-pointer region casts with `BEGIN_ALLOC_REGION`,
      `END_ALLOC_REGION`, and their `MULTI` variants
- [ ] Complete allocation-region provenance, resizing, and dynamic-region
      semantics

### Statements and control flow

- [x] Variable declarations
- [x] Assignment statements
- [x] Compound assignments
- [x] `ASSERT`, `PANIC`, `UNIMPLEMENTED`, and `COMPILATION_ERROR` statements
- [x] Goto statements
- [x] `IF`, `UNLESS`, and while statements
- [x] For loops
- [x] `RUNTIME` statement branching
- [x] Static control flow with `STATIC_IF`, `STATIC_ELSE`, and `STATIC_WHILE`
- [x] Lambda functions / closures
- [x] Defaulted function arguments
- [x] AUTO return type inference
- [x] Range-based for and container iteration
- [x] `MATCH` dispatch over unions and variants
- [x] `VISIT` block and continuation forms for variants
- [x] `RETURN_UNEQUAL` comparison chaining
- [ ] Coroutines
- [ ] Scoped with statements

### Compile-time evaluation and metaprogramming

- [x] Constexpr evaluation of expressions that result in `bool`
- [x] Function-local `STATIC` constants and `STATIC_VAR` variables, including
      `STATIC_EVAL` mutation and `SNAPSHOT`
- [x] Antestatal global `STATIC` constants for primitive values, pointers, and
      procedure pointers
- [x] Constexpr storage, placement construction, destruction, allocation, and
      deallocation checks
- [x] Function and type templates, named/positional template arguments, and
      `AUTO` parameter type deduction
- [x] Template value parameters in dependent types and return types
- [x] Alias templates
- [x] Variadic positional packs with `PACK_SIZE`, `PACK_ARG`, and
      `PACK_ARG_TYPE`
- [x] `OPTION` declarations and compile-time option lookup
- [x] `INCLUDE_IF` / `ENABLE_IF`-style conditional availability
- [x] Architecture, OS, binary-format, and environment predicates
- [x] Direct public struct field reflection with `PUBLIC_FIELD_*` operations
- [ ] Reflection beyond direct public struct fields
- [ ] Registries

### Standard library

- [x] Strings and dynamic containers with `std::dynarr`, `std::list`,
      `std::map`, `std::set`, and `std::cord`
- [x] Shared ownership with `std::shared_ptr` on native targets
- [x] Native threads with `std::thread`, join ownership, and thread-local
      destruction
- [x] Filesystem file ownership and synchronous read/write operations
- [x] Async file I/O with `std::async_context`, `std::async_file`, and
      `std::thread_pool` on Linux, Windows, and macOS

### Modules, tests, and native execution

- [x] Using multiple modules together
- [x] `ALIAS` declarations for types, namespaces, modules, globals, and functions
- [x] Declaration privacy with module, class, and named scopes
- [x] Static-test expected-failure handling for compilation failures and runtime
      assertion failures
- [x] Dual static/unit tests with `DUAL_TEST`
- [x] Architecture-selected `ASM_PROCEDURE` declarations
- [x] ASM `OBJECT_REF` operands and custom native entrypoints with
      `PROGRAM_START`, `POST_DETECT`, `UNIT_TEST_MAIN`, and `MAIN_FUNCTION_ARRAY`
- [x] Linux ELF executable generation and linking
- [x] Linux native unit-test execution through the testbundle suite
- [x] Native syscall tests for Linux targets
- [x] Native allocator entry points covered by current memory unit tests
- [x] `PER_THREAD` global storage and thread-local initguard lowering

## Compiler and runtime

- [x] Linux ELF binaries
- [x] ELF metadata preservation, symbol table generation, and readable symbol
      display names
- [x] Zero-initialized global storage and static serialoid link support
- [x] Static extern procedure declarations and linksymbol metadata
- [x] Dynamic extern imports for Linux x64/glibc (including symbol versions),
      Windows x64 DLLs, and macOS ARM64/libSystem
- [x] Configurable CPU/subarchitecture and target-feature selection, CPU tuning,
      and runtime selection between compiled CPU steppings
- [x] PE/COFF executable generation and linking for Windows x64 (native
      execution untested)
- [x] Mach-O executable generation and linking for macOS ARM64
- [x] JVM bytecode and executable JAR generation through Cortado
- [x] LLVM source-file, function, and line debug metadata
- [ ] Complete runtime unwinding support
