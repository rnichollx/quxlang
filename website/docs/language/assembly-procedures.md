# Assembly Procedures

`ASM_PROCEDURE` declares a callable body written for one architecture:

```quxlang
::linux_exit INCLUDE_IF(OS_LINUX) ASM_PROCEDURE X64
  CALLABLE(@code I32; RETURN I32)
{
  MOV RAX, 60
  SYSCALL
  RET
}
```

The architecture tag selects the assembler and validates its register and
instruction vocabulary. The accepted architecture tags are `ARM32`, `ARM64`,
`X64`, `X86`, and `Z_ARCH`.

## One logical procedure, several architectures

Several declarations may share one name when their architecture tags are
disjoint:

```quxlang
::exit ASM_PROCEDURE X64
  CALLABLE(@code I32; RETURN I32)
{
  MOV RAX, 60
  SYSCALL
  RET
}

::exit ASM_PROCEDURE ARM64
  CALLABLE(@code I32; RETURN I32)
{
  MOV X8, 93
  SVC 0
  RET
}
```

The active target selects the architecture definition. The declarations must
present a compatible logical callable surface.

## Callable ABI

`CALLABLE CALLCONV CCALL(...)` can state an explicit native calling convention.
Named arguments are part of the Quxlang call surface even when the assembly body
ultimately reads fixed ABI registers or stack positions.

Assembly procedures are target-specific declarations; guard OS-, environment-,
or runtime-specific operations with `INCLUDE_IF` as well as the architecture
tag.

## Referring to Quxlang symbols

Assembly operands use structured references instead of spelling a mangled link
name directly:

```quxlang
::start ASM_PROCEDURE X64
{
  MOVABS RAX, OFFSET OBJECT_REF(ACTIVE_STEPPING)
  MOVABS R10, OFFSET PROCEDURE_REF("", worker#[0])
  CALL R10
  RET
}
```

- `OBJECT_REF(symbol)` lowers to the link name of a global object.
- `PROCEDURE_REF("calling-convention", functanoid)` identifies one concrete
  function instantiation. An empty string selects the default convention.

The referenced function must be concrete; provide template arguments or an
instantiation index such as `#[0]` where overload resolution requires one.
Platform assemblers may attach relocation syntax to the structured reference,
such as `@PAGE`, `@PAGEOFF`, or a GOT relocation.

## Inline assembly status

`ASM_INLINE_FUNCTION` and its register-bound `CALLABLE`/`CLOBBER` surface are
not implemented.

Structured `EXTERNAL("C", "symbol")` and `EXTERNAL("LINKER", "symbol")`
operands are not implemented end to end for ARM-family assembly. Use
`EXTERN_PROCEDURE` for external calls. Use `PROCEDURE_REF` or `OBJECT_REF` for
references to reached Quxlang symbols.

Runtime entry procedures and the compiler-owned stepping arrays are documented
on [Program startup and runtime hooks](../toolchain/program-startup-and-runtime-hooks.md).
