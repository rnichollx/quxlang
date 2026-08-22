# External Procedures

`EXTERN_PROCEDURE` declares a callable symbol supplied by another binary or
runtime environment. The declaration gives the external identity, Quxlang call
shape, and calling convention.

## Declaration syntax

```quxlang
::malloc INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["glibc":"malloc" VERSION "GLIBC_2.2.5"]
  CALLABLE CALLCONV CCALL(@bytes SZ; RETURN ADDRESS);
```

The declaration consists of:

1. a library or external scope string;
2. an external symbol-name string;
3. optional configuration such as `VERSION` or `OPTIONAL`;
4. a `CALLABLE` signature;
5. an optional explicit `CALLCONV`; and
6. a terminating semicolon.

`CALLCONV CCALL` selects the C ABI. When `CALLCONV` is omitted, the declaration
uses `CCALL`.

## Parameters and return type

Parameters may be named or positional. The optional return type follows a
semicolon inside `CALLABLE`:

```quxlang
::native_operation
  EXTERN_PROCEDURE["native":"operation"]
  CALLABLE(@input I32, U64; RETURN BOOL);
```

Named parameters use the ordinary Quxlang call-argument mapping. An unnamed
parameter is supplied positionally. A declaration without `; RETURN Type` has
no result.

## Versions and optional symbols

```quxlang
::optional_extension INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["extension":"try_feature" OPTIONAL]
  CALLABLE(@value I32; RETURN I32);
```

`VERSION "name"` records the required external symbol version. `OPTIONAL`
permits the external symbol to be absent during linking or loading. It does not
make an unconditional call safe: the program remains responsible for calling
the symbol only where the target and runtime provide it.

## JVM call conventions

Managed backends use conventions that identify the represented bytecode
operation:

| Convention | External operation | Parameter shape |
| --- | --- | --- |
| `JVM_STATIC` | Invoke a static method | Ordinary parameters; optional return |
| `JVM_VIRTUAL` | Invoke a virtual method | `@THIS` plus method parameters |
| `JVM_INTERFACE` | Invoke an interface method | `@THIS` plus method parameters |
| `JVM_CONSTRUCTOR` | Construct an instance | Constructor parameters; managed-reference return |
| `JVM_GETFIELD` | Read an instance field | `@THIS`; field-value return |
| `JVM_PUTFIELD` | Write an instance field | `@THIS` and `@VALUE` |
| `JVM_GETSTATIC` | Read a static field | No receiver; field-value return |
| `JVM_PUTSTATIC` | Write a static field | `@VALUE` |

For example:

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

The declared receiver and value parameters must match the selected convention.
The returned managed-reference type is declared with
[External Types](external-types.md).

## Availability and ABI requirements

The calling convention, parameter types, result type, external identity, and
symbol version form an ABI contract. A mismatch is not repaired by the Quxlang
type system. Use `INCLUDE_IF` and target predicates so declarations are present
only on compatible targets.

See [Call Arguments](call-arguments.md),
[Target Availability](availability-and-targets.md), and
[The `qxcbuild.yml` File](toolchain/qxcbuild-file.md).
