# Backends and Layout

Quxlang separates source-language feature support from a backend's physical
layout and execution model.

## Native LLVM targets

The LLVM backend produces native objects and executables for configured native
platforms. Native compilation is the zero-overhead guarantee boundary for
Quxlang abstractions.

Native targets provide byte size and alignment for ordinary data types. Types
such as `ALIGNED_STORAGE`, native external procedure signatures, assembly
procedures, and native allocation depend on that layout.

## Cortado and the JVM

The Cortado backend produces JVM bytecode. It targets language feature parity,
but a managed runtime is not required to reproduce native representation or
zero-overhead behavior.

Some JVM values are layoutless: their representation is managed by the runtime
and does not have a source-visible fixed size or alignment. GC pointers
use `~>T`.

## Query layout explicitly

```quxlang
STATIC_IF(TYPE_IS_LAYOUTLESS(value_type))
{
  use_managed_representation();
}
STATIC_ELSE
{
  STATIC bytes SZ := SIZEOF(value_type);
  STATIC alignment SZ := ALIGNOF(value_type);
}
```

`TYPE_IS_LAYOUTLESS(T)` is the semantic query. `ARCH_IS_LAYOUTLESS` identifies
a wholly layoutless architecture category for broader target selection.

On the current JVM target, fixed-width `I8`, `U8`, `I16`, `U16`, `I32`, `U32`,
`I64`, and `U64` retain fixed byte sizes. Other widths and aggregate types may
be layoutless. `ALIGNOF` and reached `ALIGNED_STORAGE` uses require an actual
layout.

Use `RUNTIME NATIVE` when an operation has a native-only implementation and an
explicit non-native alternative.
