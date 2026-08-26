# Overview of External Procedures

External procedures let Quxlang call functions, methods, constructors, and
field operations supplied by a native library or managed runtime.

## Call a native symbol

```quxlang
::malloc INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["glibc":"malloc" VERSION "GLIBC_2.2.5"]
  CALLABLE CALLCONV CCALL(@bytes SZ; RETURN ADDRESS);
```

The bracket identifies the library and symbol. `CALLABLE` describes the
Quxlang parameters and result. `CALLCONV CCALL` selects the C calling
convention, while `VERSION` records a required symbol version.

## Declare an optional symbol

```quxlang
::optional_extension INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["extension":"try_feature" OPTIONAL]
  CALLABLE(@value I32; RETURN I32);
```

`OPTIONAL` permits the symbol to be missing from the external environment. It
does not make an unconditional call valid, so code must still establish that
the selected target and runtime provide the symbol before calling it.

## Describe managed-runtime operations

```quxlang
::system_out INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_PROCEDURE["java/lang/System":"out"]
  CALLABLE CALLCONV JVM_GETSTATIC(; RETURN ~>java_print_stream);

::java_point_constructor INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_PROCEDURE["java/awt/Point":"<init>"]
  CALLABLE CALLCONV JVM_CONSTRUCTOR(
    @x I32, @y I32; RETURN ~>java_point
  );
```

Other JVM conventions cover static, virtual, and interface calls plus instance
and static field reads and writes. Their `@THIS` and `@VALUE` parameters state
which object or value participates in the operation.

External procedure declarations are ABI contracts. Guard them with the target
predicate that provides the library, runtime, and calling convention.

See [External Types](external-types.md) for managed object types and
[Call Arguments](call-arguments.md) for named arguments.

## Reference

See the [External Procedures Reference](../reference/external-procedures.md)
for declaration grammar, ABI options, and every managed calling convention.
