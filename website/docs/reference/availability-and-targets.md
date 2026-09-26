# Target Availability

Target availability is expressed through compile-time target predicates,
declaration-level `INCLUDE_IF`, and runtime `HAVE_*` CPU queries.

## `INCLUDE_IF`

```text
::name INCLUDE_IF(constant-expression) declaration
.name INCLUDE_IF(constant-expression) member-declaration
```

The condition must produce a compile-time `BOOL` for the active target. When it
is `FALSE`, the compiler omits the declaration for that target.

```quxlang
::word_size INCLUDE_IF(ARCH_IS_X64) FUNCTION(): I32
{
  RETURN 64;
}

::conditional_fields STRUCT
{
  .compact INCLUDE_IF(TRUE) VAR BYTE;
  .wide INCLUDE_IF(FALSE) VAR U64;
}
```

An excluded declaration does not participate in name lookup, overload
resolution, field layout, interface discovery, test discovery, or other
compiler operations. Code cannot refer to it for that target.

`INCLUDE_IF` controls declarations. [`STATIC_IF`](conditional-statements.md)
selects statements inside an existing declaration; an unselected
`STATIC_IF` branch is discarded during semantic generation.

## Compile-time target predicates

The compiler provides these predicate families as compile-time `BOOL` values:

| Category | Predicates |
| --- | --- |
| Architecture | `ARCH_IS_X64`, `ARCH_IS_X86`, `ARCH_IS_ARM32`, `ARCH_IS_ARM64`, `ARCH_IS_RISCV64`, `ARCH_IS_Z_ARCH`, `ARCH_IS_JVM`, `ARCH_IS_LAYOUTLESS` |
| Backend | `BACKEND_LLVM`, `BACKEND_CORTADO` |
| Operating system | `OS_LINUX`, `OS_WINDOWS`, `OS_MACOS` |
| Binary format | `BINARY_ELF`, `BINARY_MACHO`, `BINARY_PE`, `BINARY_WASM` |
| Environment | `ENVIRONMENT_IS_GLIBC`, `ENVIRONMENT_IS_MUSL`, `ENVIRONMENT_IS_BIONIC`, `ENVIRONMENT_IS_MSVC`, `ENVIRONMENT_IS_UCRT`, `ENVIRONMENT_IS_CYGWIN`, `ENVIRONMENT_IS_STATIC`, `ENVIRONMENT_IS_LIBSYSTEM`, `ENVIRONMENT_IS_FREESTANDING` |
| Unwind format | `UNWIND_FORMAT_IS_NONE`, `UNWIND_FORMAT_IS_DWARF_EH_FRAME`, `UNWIND_FORMAT_IS_ARM_EHABI`, `UNWIND_FORMAT_IS_WINDOWS_SEH`, `UNWIND_FORMAT_IS_SJLJ`, `UNWIND_FORMAT_IS_WASM` |

Predicates may be combined with ordinary compile-time Boolean operators:

```quxlang
::hosted_linux_operation INCLUDE_IF(
  OS_LINUX && ENVIRONMENT_IS_FREESTANDING == FALSE
) FUNCTION()
{
}
```

Each concrete `ARCH_IS_*` predicate identifies the configured target
architecture. `ARCH_IS_LAYOUTLESS` is `TRUE` when the managed runtime controls
type representations and those types have no fixed byte layout visible to
Quxlang code.

The capability registry reserves `ARCH_IS_RISCV64` and RISC-V capability names.
The current public `qxcbuild.yml` target loader does not yet accept a RISC-V
target.

## Runtime CPU capability queries

A source expression named `HAVE_<CAPABILITY>` returns a runtime `BOOL`:

```quxlang
::avx2_available FUNCTION(): BOOL
{
  RETURN HAVE_X64_FEATURE_AVX2;
}
```

The capability name uses the stable identifiers configured in target
`steppings`, such as `X64_FEATURE_AVX2` or `X64_FEATURE_SSE4_1`. Aggregate
queries such as `HAVE_X64_FEATURES_V3` require every constituent capability.

| Query context | Result generation |
| --- | --- |
| Matching CPU family, capability fixed by the current stepping | LLVM emits the known Boolean value |
| Matching CPU family, capability detected at startup | The expression reads the compiler-owned `_ENABLED` state |
| Different CPU family | `FALSE` |
| Aggregate capability | Conjunction of its constituent capability results |

The target's stepping sequence must contain each queried capability whose
detection routine is required. Startup selects the highest compatible stepping
and uses code compiled for that same stepping. The complete configuration and
constant-folding contract is in
[CPU Capabilities and Steppings](cpu-capabilities-and-steppings.md).

`HAVE_*` is a runtime expression. It is not accepted as the constant condition
of `INCLUDE_IF` or `STATIC_IF`.

## Availability examples

An architecture-specific assembly body normally combines architecture and
operating-system availability:

```quxlang
::linux_exit INCLUDE_IF(OS_LINUX) ASM_PROCEDURE X64
{
  MOV RAX, 60
  SYSCALL
  RET
}
```

Inheritance is available during constant evaluation and on native targets. The
current JVM backend does not implement it. A declaration that exercises native
inheritance can be excluded with:

```quxlang
::native_hierarchy_test INCLUDE_IF(ARCH_IS_JVM!!) DUAL_TEST
{
  // Exercise the hierarchy here.
}
```

## Unsupported expressions

`TARGET("name")`, kernel predicates, and `OS_BSD` are not implemented. Target
selection remains part of bundle configuration; source declarations can use
the predicate families listed above.

[Backends and Layout](backends-and-layout.md) specifies layout-dependent
language features. [`qxcbuild.yml`](qxcbuild-file.md) specifies target
configuration.
