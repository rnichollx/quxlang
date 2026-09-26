# Overview of Runtime Selection

Runtime selection lets one function contain different implementations for
compile-time execution, native execution, and an alternate execution mode.

## Compile-time and ordinary execution

The `RUNTIME CONSTEXPR` form provides separate implementations for compile-time
evaluation and ordinary program execution:

```quxlang
::allocate TEMPLATE(@T TYPE AUTO) FUNCTION(): -> TYPED_STORAGE(T)
{
  RUNTIME CONSTEXPR
  {
    RETURN CONSTEXPR_ALLOC#T();
  }
  ELSE
  {
    RETURN allocate_native#T();
  }
}
```

The first block runs when `allocate` is being evaluated at compile time. The
`ELSE` block runs during ordinary execution.

## Native and alternate implementations

The `RUNTIME NATIVE` form isolates a native implementation:

```quxlang
RUNTIME NATIVE
{
  release_native(@address address);
}
ELSE
{
  release_portable(@address address);
}
```

If `ELSE` is omitted, the statement does nothing in the other mode.

## Feature selection

`RUNTIME` chooses by execution mode. Ordinary `IF` evaluates a runtime
condition. `STATIC_IF` evaluates a compile-time expression that determines
which source body is generated.

## Reference

For branch validity, dependency selection, variable state, and control-flow
rules, see the [Runtime Selection Reference](../reference/runtime-selection.md).
