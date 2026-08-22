# Overview of External Types

External types let Quxlang name types that belong to another runtime even
though Quxlang does not define their object layout. They are especially useful
for managed-runtime classes.

## Declare an external type

```quxlang
::java_object INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.base":"java/lang/Object"];

::java_point INCLUDE_IF(ARCH_IS_JVM)
  EXTERN_TYPE["java.desktop":"java/awt/Point"];
```

The first string names the external scope or library, and the second names the
type within it. The Quxlang name before `EXTERN_TYPE` is what source code uses.

Guard platform-specific declarations with `INCLUDE_IF` so other targets do not
see a type they cannot provide.

## Hold managed objects

Managed runtimes use `~>T` references:

```quxlang
VAR object ~>java_object;
ASSERT((object??) == FALSE);
```

A default managed reference is null. `??` tests whether it carries an object.

## Perform checked casts

```quxlang
VAR object ~>java_object := stream AS CHECKED ~>java_object;
VAR checked_stream ~>java_print_stream :=
  object AS CHECKED ~>java_print_stream;
```

Checked conversions ask the managed runtime to validate the target type. The
external type remains opaque: its usable operations come from external
procedure declarations, not Quxlang fields inferred from its external name.

See [External Procedures](external-procedures.md) for invoking runtime methods
and fields.

## Complete technical rules

See the [External Types Reference](../../reference/external-types.md) for exact
declaration, reference, casting, and layout rules.
