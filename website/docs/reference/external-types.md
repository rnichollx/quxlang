# External Types

`EXTERN_TYPE` declares a nominal type supplied by an external runtime. Quxlang
knows the type's external identity but does not define its object layout.

## Declaration syntax

```quxlang
::java_object INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.base":"java/lang/Object"];

::java_point INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.desktop":"java/awt/Point"];
```

The grammar is:

```text
EXTERN_TYPE["external scope":"external type name"];
```

Both external names are string literals. The Quxlang declaration name remains
the name used in Quxlang type expressions. The terminating semicolon is
required.

Because the type belongs to a particular runtime or ABI, its declaration should
normally carry an `INCLUDE_IF` predicate that excludes it from incompatible
targets.

## Managed references

On managed targets, `~>T` is a managed reference to an external type:

```quxlang
VAR object ~>java_object;
ASSERT((object??) == FALSE);
```

The default value is null. `value??` tests for an object and `value?!` tests for
null.

## Checked conversions

`AS CHECKED ~>Target` requests a managed-reference conversion with runtime type
validation:

```quxlang
VAR object ~>java_object := stream AS CHECKED ~>java_object;
VAR checked_stream ~>java_print_stream :=
  object AS CHECKED ~>java_print_stream;
```

The target must support the external type relationship and checked-cast
operation. A failed checked conversion follows the managed backend's checked
cast failure semantics.

## Layout boundary

An external type is opaque to Quxlang layout computation. It is not a Quxlang
`STRUCT`, and declaring it does not provide fields, constructors, or member
functions. Operations on its values are supplied through managed-reference
operations and [External Procedures](external-procedures.md).

See [Target Availability](availability-and-targets.md) and
[Backends and Layout](toolchain/backends-and-layout.md).
