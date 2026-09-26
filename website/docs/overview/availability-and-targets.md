# Overview of Target Availability

Quxlang provides `INCLUDE_IF` for declarations that are available only on
specific configured targets. The following system-call wrapper is present on
Linux targets and absent from other targets:

```quxlang
::process_identifier INCLUDE_IF(OS_LINUX) FUNCTION(): I32
{
  RETURN linux_getpid();
}
```

The compiler evaluates the condition for each target. An excluded declaration
is unavailable to name lookup and overload resolution and occupies no storage.
This makes `INCLUDE_IF` appropriate for declarations whose types, instructions,
libraries, or layouts are unavailable on another target.

Common predicates describe the architecture, backend, operating system, binary
format, environment, and unwind format:

```quxlang
::native_word_size INCLUDE_IF(ARCH_IS_LAYOUTLESS == FALSE) FUNCTION(): SZ
{
  RETURN BYTES(SZ);
}
```

`STATIC_IF` provides statement-level target selection when the enclosing
declaration remains available on every target.

## Runtime selection of CPU-specific code

`HAVE_*` expressions report whether the current machine supports a configured
CPU capability.

```quxlang
::use_vector_path FUNCTION(): BOOL
{
  RETURN HAVE_X64_FEATURE_AVX2;
}
```

These queries allow one executable to select among compiled CPU steppings at
startup. When a stepping specifies a capability, the backend can replace an
individual query with a constant; otherwise the query reads the detected
capability state. Runtime detection and additional compiled steppings increase
startup work and binary size, so configure capabilities that the program uses.

The [Target Availability Reference](../reference/availability-and-targets.md)
lists every predicate family and the exact inclusion and runtime-query rules.
