# Target Availability

`INCLUDE_IF(condition)` controls whether a declaration exists in the active
source configuration.

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

An excluded declaration does not participate in lookup, overload resolution,
field layout, interface discovery, tests, or other declaration consumers. This
is different from a runtime branch around an existing declaration.

## Target predicates

The complete currently generated predicate families are:

| Category | Predicates |
| --- | --- |
| Architecture | `ARCH_IS_X64`, `ARCH_IS_X86`, `ARCH_IS_ARM32`, `ARCH_IS_ARM64`, `ARCH_IS_RISCV64`, `ARCH_IS_Z_ARCH`, `ARCH_IS_JVM`, `ARCH_IS_LAYOUTLESS` |
| Backend | `BACKEND_LLVM`, `BACKEND_CORTADO` |
| Operating system | `OS_LINUX`, `OS_WINDOWS`, `OS_MACOS` |
| Binary format | `BINARY_ELF`, `BINARY_MACHO`, `BINARY_PE`, `BINARY_WASM` |
| Environment | `ENVIRONMENT_IS_GLIBC`, `ENVIRONMENT_IS_MUSL`, `ENVIRONMENT_IS_BIONIC`, `ENVIRONMENT_IS_MSVC`, `ENVIRONMENT_IS_UCRT`, `ENVIRONMENT_IS_CYGWIN`, `ENVIRONMENT_IS_STATIC`, `ENVIRONMENT_IS_LIBSYSTEM`, `ENVIRONMENT_IS_FREESTANDING` |
| Unwind format | `UNWIND_FORMAT_IS_NONE`, `UNWIND_FORMAT_IS_DWARF_EH_FRAME`, `UNWIND_FORMAT_IS_ARM_EHABI`, `UNWIND_FORMAT_IS_WINDOWS_SEH`, `UNWIND_FORMAT_IS_SJLJ`, `UNWIND_FORMAT_IS_WASM` |

`ARCH_IS_LAYOUTLESS` tests the architecture's layout model rather than one
spelling from the target file. The current public bundle loader does not yet
accept a RISC-V target even though the source predicate and capability registry
reserve `ARCH_IS_RISCV64`.

Predicates are ordinary compile-time `BOOL` values and can be combined:

```quxlang
::hosted_thread_function INCLUDE_IF(
  (OS_LINUX || OS_WINDOWS || OS_MACOS) &&
  ENVIRONMENT_IS_FREESTANDING == FALSE
) FUNCTION()
{
}
```

## CPU capabilities

Configured CPU capability stems use names such as `X64_FEATURE_AVX2` and
`X64_FEATURE_SSE4_1`. Put these stems in the target's `steppings`
configuration. Each stepping is compiled with its configured capabilities,
and startup selects the highest compatible stepping at runtime.

Stable capability names are backend-neutral. Target configuration may also use
aggregate levels such as `X64_FEATURES_V1` through `X64_FEATURES_V4`.

Source-level `HAVE_*` expressions are runtime `BOOL` queries:

```quxlang
::avx2_available FUNCTION(): BOOL
{
  RETURN HAVE_X64_FEATURE_AVX2;
}
```

On the matching CPU family, an individual query reads its compiler-owned
detected `_ENABLED` flag. On another CPU family it is `FALSE`. Aggregate queries
such as `HAVE_X64_FEATURES_V3` require every constituent capability. LLVM emits
a boolean constant when the current stepping fixes an individual capability;
otherwise it loads the detected flag. Configure queried capabilities in the
target's stepping sequence so the runtime detector is included.

`HAVE_*` is not a constexpr expression and cannot be used with `STATIC_IF` or
`INCLUDE_IF`. See
[CPU capabilities and steppings](toolchain/cpu-capabilities-and-steppings.md)
for detection, aggregate, and constant-folding details.

Use `STATIC_IF` to select statements using an ordinary compile-time expression.
Use `INCLUDE_IF` when a declaration itself must be absent.

## Unimplemented target expressions

`TARGET("name")`, the kernel predicates, and `OS_BSD` are not implemented. Use
the predicate families listed above or select the target in bundle
configuration.
