# Assembly Procedures

`ASM_PROCEDURE` declares a callable procedure whose body uses the instructions
and registers of one architecture.

## Syntax

```text
::name ASM_PROCEDURE architecture callable-interface... {
  instruction...
}

callable-interface:
  CALLABLE [CALLCONV calling-convention] [NOEXCEPT]
    ([parameter, ...] [; RETURN type])

parameter:
  type
  @api-name type
```

The supported architecture tags are:

| Tag | Instruction set |
| --- | --- |
| `X64` | 64-bit x86 |
| `X86` | 32-bit x86 |
| `ARM64` | 64-bit Arm |
| `ARM32` | 32-bit Arm |
| `Z_ARCH` | IBM Z architecture |

The architecture name is exact. A generic `ARM` tag is not accepted.

## Callable interfaces

A declaration can contain zero or more `CALLABLE` interfaces before its body.
Each interface exposes the procedure as a typed Quxlang callable. Omitting
`CALLCONV` selects `CCALL`.

```quxlang
::write_bytes ASM_PROCEDURE X64
  CALLABLE CALLCONV CCALL(
    @descriptor I32,
    @buffer CONST=>>BYTE,
    @count SZ;
    RETURN SZ
  )
{
  MOV RAX, 1
  SYSCALL
  RET
}
```

Parameters may be named with `@name` or positional. Argument and result
registers are determined by the calling convention; register-bound parameters
are not part of the `ASM_PROCEDURE` syntax. `NOEXCEPT` requires the procedure
to handle exceptions before they escape.

Named parameters belong to the Quxlang call interface. The assembly body reads
the locations assigned by its ABI and does not refer to those source parameter
names.

## Declaration selection

Several declarations may use the same Quxlang name when each declaration
targets a different architecture:

```quxlang
::processor_id ASM_PROCEDURE X64
  CALLABLE(; RETURN I32)
{
  MOV EAX, 64
  RET
}

::processor_id ASM_PROCEDURE ARM64
  CALLABLE(; RETURN I32)
{
  MOV X0, 64
  RET
}
```

The active target selects the matching architecture declaration. Definitions
that share a name must provide compatible callable interfaces. Architecture
selection does not express operating-system or environment availability; apply
`INCLUDE_IF` for those conditions.

## Body syntax

The body accepts registers, immediates, labels, and operands supported by the
selected architecture parser. A label uses the `LABEL` form:

```quxlang
::retrying_operation ASM_PROCEDURE X64
{
  LABEL retry;
  SYSCALL
  JA retry
  RET
}
```

The compiler checks each instruction and operand against the selected
architecture. Platform-specific relocation suffixes remain part of the
instruction operand where supported.

## Quxlang symbol operands

Assembly names Quxlang entities through structured operands:

| Operand | Requirement | Result |
| --- | --- | --- |
| `OBJECT_REF(symbol)` | `symbol` resolves to a global object | The object's emitted link name |
| `PROCEDURE_REF("calling-convention", function-selector)` | The second argument resolves to one concrete callable instantiation | The procedure's emitted link name |

```quxlang
::start ASM_PROCEDURE X64
{
  MOVABS RAX, OFFSET OBJECT_REF(ACTIVE_STEPPING)
  MOVABS R10, OFFSET PROCEDURE_REF("", worker!$[0])
  CALL R10
  RET
}
```

An empty string in `PROCEDURE_REF` selects the default calling convention. The
target must be concrete: supply template arguments and, when necessary, a
zero-based overload identifier such as `!$[0]`. An unresolved symbol, a
non-global `OBJECT_REF`, or an uninstantiated or noncallable `PROCEDURE_REF`
causes compilation to fail.

Relocation spelling depends on the object format and architecture. For
example, Mach-O ARM64 code may use `OBJECT_REF(name)@PAGE` together with
`OBJECT_REF(name)@PAGEOFF`.

## Execution and target support

An `ASM_PROCEDURE` is available only in native runtime code. It cannot be
invoked during constant evaluation. The current Cortado JVM backend reports a
lowering error when a program calls one.

The architecture tag determines which instructions and registers are valid. It
does not by itself establish that an operating-system service, calling
convention, or relocation is available. The corresponding
[target predicate](availability-and-targets.md) expresses those additional
availability constraints.

## Implementation status

`ASM_INLINE_FUNCTION` and its register-bound `CALLABLE` and `CLOBBER` syntax
are parsed but are not implemented by the backends.

Structured `EXTERNAL("C", "symbol")` and `EXTERNAL("LINKER", "symbol")`
operands are not implemented end to end for ARM-family assembly. An
[external procedure](external-procedures.md) represents an external call.
`PROCEDURE_REF` and `OBJECT_REF` represent Quxlang symbols emitted in the
output.

Runtime entry procedures and compiler-owned stepping arrays are specified in
[Program Startup and Runtime Hooks](program-startup-and-runtime-hooks.md).
