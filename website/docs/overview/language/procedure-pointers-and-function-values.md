# Overview of Procedure Pointers and Function Values

Quxlang has two ways to pass callable behavior: a procedure pointer is a
runtime address with a fixed signature, while a function value preserves the
identity of a selected Quxlang function during generic compilation.

## Store a runtime procedure pointer

```quxlang
::is_positive FUNCTION(@value I32): BOOL
{
  RETURN value > 0;
}

VAR predicate CONST->PROCEDURE(@value I32: BOOL) := is_positive<-;
VAR accepted BOOL := predicate(@value 4);
```

The `PROCEDURE` type records parameter types, API names, and the result type.
Use it for callbacks and ABI boundaries where a runtime function address is
required.

```quxlang
VAR native_callback CONST->PROCEDURE CCALL NOEXCEPT(I32: I32);
```

Calling-convention and `NOEXCEPT` qualifiers are part of that runtime type.

## Pass a compile-time function value

```quxlang
::invoke FUNCTION(@callable AUTO(fn), @value I32): BOOL
{
  RETURN callable(@value value);
}

VAR accepted BOOL := invoke(@callable is_positive, @value 4);
```

`AUTO(fn)` binds the chosen callable without first erasing it to a runtime
address. This also works for a bound member such as `object.method`, which
retains its receiver.

Use a function value for generic composition and a procedure pointer for a
fixed runtime representation. A bound member does not currently convert to a
procedure pointer because that type has no captured-receiver slot.

## Reference

See the [Procedure Pointers and Function Values Reference](../../reference/procedure-pointers-and-function-values.md)
for named and positional signatures, qualifiers, deduction compatibility, and
conversion limitations.
