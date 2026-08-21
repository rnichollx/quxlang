# Procedure Pointers and Function Values

Quxlang distinguishes a runtime procedure signature from a compile-time
function value.

## Procedure pointers

```quxlang
::sum FUNCTION(%left I32, %right I32): I32
{
  RETURN left + right;
}

::call_sum FUNCTION(): I32
{
  VAR procedure CONST->PROCEDURE(I32, I32: I32) := sum<-;
  RETURN procedure(% [4, 5]);
}
```

`PROCEDURE(I32, I32: I32)` has two positional `I32` parameters and returns
`I32`. Named parameters keep their API names inside the procedure type:

```quxlang
VAR callback CONST->PROCEDURE(@value I32: BOOL);
```

`CCALL`, `STDCALL`, and `NOEXCEPT` qualify the type before its parameter list:

```quxlang
VAR native_callback CONST->PROCEDURE CCALL NOEXCEPT(I32: I32);
```

## Compile-time function values

`AUTO(name)` can bind a function value while preserving its compile-time
identity:

```quxlang
::invoke FUNCTION(%callable AUTO(fn), %left I32, %right I32): I32
{
  RETURN callable(% [left, right]);
}

VAR result I32 := invoke(% [sum, 3, 4]);
```

Several parameters using the same `AUTO(fn)` name must bind compatible function
values. A bound member such as `object.method` can also be passed this way.

Use a procedure pointer when a runtime address and fixed ABI are required. Use
a function value when generic code should retain the selected Quxlang function
or bound member as a compile-time value.

A free function can be converted to a procedure pointer with `<-`. A bound
member may be retained as a compile-time function value, but conversion of a
bound member to a runtime procedure pointer is not currently supported because
the procedure type has no captured receiver slot.
