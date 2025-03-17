# Quxlang Resolvers

## asm_procedure_from_symbol

**Input:** `quxlang::type_symbol`: the input type symbol.

**Output:** `quxlang::asm_procedure`: the procedure at this symbol.

**Throws:** If the symbol is not an asm procedure.

## called_functanoids

**Input:** `quxlang::type_symbol`: reference to a functanoid or procedure.

**Output:** `std::set<quxlang::type_symbol>`: the functanoids called by this functanoid.

## callee_temploid_selection

**Input** `quxlang::instanciation_reference` An instanciation reference.

**Input.callee** `quxlang::type_symbol` The callee function or functum.

**Input.parameters** `vector<quxlang::type_symbol>` The _nominal parameters_ of the instantiation.

**Output** `quxlang::instanciation_reference` The instanciation reference is converted from nominal parameters to formal
parameters and the callee is replaced with a selection reference if it is not already one.

The difference between nominal and formal parameters is that nominal parameters are the parameters of the _function
call_ whereas formal parameters are the parameters of the _functanoid invocation_.

Suppose for example a functum is defined with 2 functions as so:

```quxlang
::foo FUNCTION(I64, I32)
{

}

::foo FUNCTION(I64, -> T(t1))
{

}
```

We might call this funcanoid in an expression like:

```foo(a, &b)```

Supposing `a` is an `I32` and `b` is an `I32`, the input to selection would be `foo#(I32, ->I32)`.

Selection would check `#(I32, ->I32)` against `FUNCTION(I64, I32)`, which fails, and `FUNCTION(I64, -> T(t1))` which
matches. this produces the selection reference `foo#[I64, ->T(t1)]`. We then apply the _formal parameters_ to
produce `foo#[I64, ->T(t1)](I64, ->I32)`. Notice how the temploidic parameter is replaced with the parameter, but the
implicit conversion is not applied in the formal parameters. This is because template arguments is passed down to the
invoked functanoid as-is, but implicit conversions to concrete types occur during the call expression prior to the
invocation of the functanoid.

For additional illustration:

```quxlang
::foo FUNCTION(I32, T(t2)) { ... }
::v32 VAR I32;
::v64 VAR I64;
```

**`foo(v32, v32)`**:
Directly invokes `foo#[I32, T(t2)]#(I32, I32)` using copies of v32 and v32.

**`foo(v64, v32)`**:

This call creates a conversion of the first argument to `I32` and then invokes `foo#[I32, T(t2)]#(I32, I32)` using the
converted value and the original `v32`.

**`foo(v32, v64)`**:

This does not perform any conversions and directly invokes `foo#[I32, T(t2)]#(I32, I64)` using the v32 and v64 values.

**`foo(v64, v64)`**:

This call creates a conversion of the first argument to `I32` and then invokes `foo#[I32, T(t2)]#(I32, I64)` using the
converted value and the original `v64`.

## callee_temploid_instanciation

Similar to callee_temploid_selection, but also applies the instanciation to the callee.

## class_destructor_reset_elision(type) -> bool

True if the type can elide the destructor after a reset operation.

## class_trivially_resettable(type) -> bool

A type is trivally resettable if it can be reset setting all bits to 0.

## class_trivially_relocatable(type) -> bool

True if a type can be relocated by copying bits only.

## class_trivially_movable(type) -> bool

A type is trivially movable if it can be moved by copying bits and resetting all fields or setting all bits to zero.

## class_trivially

## class_contiguous(type) -> bool

True if the class layout does not contain holes. If a type is contiguous and trivially resettable, it can be reset by
using memset-like zeroing operations.

In Quxlang, types can have holes, for example, supposing we have nested structs:

```quxlang
::foo CLASS
{
  .baz VAR I32;
  .bar VAR I8;
}

::buz CLASS
{
  .x VAR foo;
  .y VAR I32;
}
```

The `buz` class in this case will be laid out like this:

```
00 | buz.x(foo).baz(I32)
01 | buz.x(foo).baz(I32)
02 | buz.x(foo).baz(I32)
03 | buz.x(foo).baz(I32)
04 | buz.x(foo).bar(I8)
05 |  
06 |  
07 |  
08 | buz.y(I32)
09 | buz.y(I32)
0A | buz.y(I32)
0B | buz.y(I32)
```

In this case, the `buz` class is not contiguous because there are holes between the `foo` and `y` fields for alignment.
In C and C++, this is solved with "padding". However, unlike C and C++, Quxlang produces "holes" instead of padding. The
difference is illustrated in the following example:

```quxlang
::bif CLASS
{
  .w VAR buz;
  .u VAR I16;
}
```

Might have a layout like this:

```
00 | bif.w(buz).x(foo).baz(I32)
01 | bif.w(buz).x(foo).baz(I32)
02 | bif.w(buz).x(foo).baz(I32)
03 | bif.w(buz).x(foo).baz(I32)
04 | bif.w(buz).x(foo).bar(I8)
05 |
06 | bif.u(I16)
07 | bif.u(I16)
08 | bif.w(buz).y(I32)
09 | bif.w(buz).y(I32)
0A | bif.w(buz).y(I32)
0B | bif.w(buz).y(I32)
```

In this case, we can see that the bif class has the same size as the buz class which is a member of bif, because the u
variable filled a hole in the layout of the buz class. In general, it is not safe to use memset to initialize objects in
Quxlang because of this behavior. Instead, the reset operation should be used.

    