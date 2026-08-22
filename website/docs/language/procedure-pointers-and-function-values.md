# Procedure Pointers and Function Values

Quxlang distinguishes a runtime procedure signature from a compile-time
function value.

## Procedure pointers

```quxlang
::is_in_range FUNCTION(@value I32, @minimum I32, @maximum I32): BOOL
{
  RETURN value >= minimum && value <= maximum;
}

::check_percentage FUNCTION(): BOOL
{
  VAR procedure CONST->PROCEDURE(
    @value I32,
    @minimum I32,
    @maximum I32: BOOL
  ) := is_in_range<-;
  RETURN procedure(@value 75, @minimum 0, @maximum 100);
}
```

Named parameters keep their API names inside the procedure type. Positional
procedure parameters are also available when required by the API or ABI:

```quxlang
VAR positional_callback CONST->PROCEDURE(I32, I32: I32);
```

`CCALL`, `STDCALL`, and `NOEXCEPT` qualify the type before its parameter list:

```quxlang
VAR native_callback CONST->PROCEDURE CCALL NOEXCEPT(I32: I32);
```

## Compile-time function values

`AUTO(name)` can bind a function value while preserving its compile-time
identity:

```quxlang
::invoke_range_check FUNCTION(
  @callable AUTO(fn),
  @value I32,
  @minimum I32,
  @maximum I32
): BOOL
{
  RETURN callable(@value value, @minimum minimum, @maximum maximum);
}

VAR result BOOL := invoke_range_check(
  @callable is_in_range,
  @value 75,
  @minimum 0,
  @maximum 100
);
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
