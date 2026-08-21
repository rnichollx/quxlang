# External Types and Procedures

External declarations describe types and symbols supplied by another runtime or
binary environment.

## External types

```quxlang
::java_object INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.base":"java/lang/Object"];

::java_point INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.desktop":"java/awt/Point"];

::java_print_stream INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.base":"java/io/PrintStream"];
```

The strings identify the external scope and type. A layoutless runtime can use
managed `~>` references to such a type.

## External procedures

```quxlang
::malloc INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["glibc":"malloc" VERSION "GLIBC_2.2.5"]
  CALLABLE CALLCONV CCALL(@bytes SZ; RETURN ADDRESS);

::optional_extension INCLUDE_IF(ENVIRONMENT_IS_GLIBC)
  EXTERN_PROCEDURE["extension":"try_feature" OPTIONAL]
  CALLABLE CALLCONV CCALL(@value I32; RETURN I32);
```

The configuration bracket contains the library and symbol names. `VERSION`
records a required symbol version. `OPTIONAL` may appear in the same bracket and
marks a symbol whose absence is permitted by the declaration. Code using an
optional symbol remains responsible for a target-appropriate availability
contract.

`CALLABLE` defines named and positional parameters, then separates an optional
return type with `; RETURN Type`. `CALLCONV` selects the ABI, such as `CCALL`.

## Managed-runtime call conventions

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

The JVM conventions describe the bytecode operation represented by the
declaration:

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

Their parameters use the same Quxlang named-argument model as other callables.

## Managed references and casts

`~>T` is a managed reference. Its default value is null, and the booliation
operators test whether it carries an object:

```quxlang
VAR stream ~>java_print_stream := system_out();
VAR missing ~>java_print_stream;

ASSERT(stream??);
ASSERT((missing??) == FALSE);

VAR object ~>java_object := stream AS CHECKED ~>java_object;
VAR checked_stream ~>java_print_stream :=
  object AS CHECKED ~>java_print_stream;
ASSERT(checked_stream == stream);
```

`AS CHECKED ~>Target` performs a checked managed-reference conversion. It is
appropriate both for an upcast whose validity should remain explicit and for a
downcast that requires runtime validation.

External declarations should normally be guarded by the target predicate that
makes their runtime and ABI available.

See [References and pointers](references-and-pointers.md) and
[Backends and layout](../toolchain/backends-and-layout.md).
