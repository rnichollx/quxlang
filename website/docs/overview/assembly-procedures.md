# Overview of Assembly Procedures

Quxlang provides the `ASM_PROCEDURE` keyword for declaring assembly procedures.
Assembly procedures allow the programmer precise control over the exact machine
instructions emitted by the program. System-call wrappers and early runtime
startup are typical applications:

```quxlang
::exit_process INCLUDE_IF(OS_LINUX) ASM_PROCEDURE X64
  CALLABLE(@code I32)
{
  MOV RAX, 60
  SYSCALL
  RET
}
```

The `CALLABLE` clause defines the typed interface visible to Quxlang code. The
assembly body follows the platform ABI, which determines the register containing
`code` in this example.

## Architecture-specific definitions

The same procedure can have a separate definition for each architecture:

```quxlang
::exit_process INCLUDE_IF(OS_LINUX) ASM_PROCEDURE ARM64
  CALLABLE(@code I32)
{
  MOV X8, 93
  SVC 0
  RET
}
```

The active target selects the matching declaration. An `INCLUDE_IF` condition
is required when the operation also depends on an operating system,
environment, or binary format.

Assembly provides exact instruction control and requires a separate
implementation for every supported architecture. Assembly procedures are
primarily applicable to small platform-specific operations. Portable Quxlang
code permits the backend to select and optimize instructions for each target.

Assembly can refer to emitted Quxlang procedures and global objects through
`PROCEDURE_REF` and `OBJECT_REF`; those forms avoid spelling generated linker
names.

The [Assembly Procedures Reference](../reference/assembly-procedures.md) lists
the declaration grammar, callable forms, symbolic operands, architecture
support, and current implementation limits.
