# Overview of Runtime Selection

Runtime selection lets one function contain different implementations for
compile-time execution, native execution, and an alternate execution mode.

## Compile-time and ordinary execution

Use `RUNTIME CONSTEXPR` when the compile-time evaluator needs a different
implementation from the finished program:

```quxlang
::allocate TEMPLATE(@T TYPE AUTO(t)) FUNCTION(): -> TYPED_STORAGE(t)
{
  RUNTIME CONSTEXPR
  {
    RETURN CONSTEXPR_ALLOC#t();
  }
  ELSE
  {
    RETURN allocate_native#t();
  }
}
```

The first block runs when `allocate` is being evaluated at compile time. The
`ELSE` block runs during ordinary execution.

## Native and alternate implementations

Use `RUNTIME NATIVE` to isolate a native implementation:

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

## Choosing the right feature

`RUNTIME` chooses by execution mode. It does not test a variable. Use ordinary
`IF` for a runtime condition and `STATIC_IF` for a compile-time expression that
decides which source body is generated.

## Reference

For branch validity, dependency selection, variable state, and control-flow
rules, see the [Runtime Selection Reference](../../reference/runtime-selection.md).
