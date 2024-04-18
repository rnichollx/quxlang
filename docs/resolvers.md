# Qux Resolvers

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

```qux
::foo FUNCTION(I64, I32)
{

}

::foo FUNCTION(I64, -> T(t1))
{

}
```

We might call this funcanoid in an expression like:

```foo(a, &b)```

Supposing `a` is an `I32` and `b` is an `I32`, the input to selection would be `foo@(I32, ->I32)`.

Selection would check `@(I32, ->I32)` against `FUNCTION(I64, I32)`, which fails, and `FUNCTION(I64, -> T(t1))` which
matches. this produces the selection reference `foo@[I64, ->T(t1)]`. We then apply the _formal parameters_ to
produce `foo@[I64, ->T(t1)](I64, ->I32)`. Notice how the temploidic parameter is replaced with the parameter, but the
implicit conversion is not applied in the formal parameters. This is because template arguments is passed down to the
invoked functanoid as-is, but implicit conversions to concrete types occur during the call expression prior to the
invocation of the functanoid.

For additional illustration:

```qux
::foo FUNCTION(I32, T(t2)) { ... }
::v32 VAR I32;
::v64 VAR I64;
```

**`foo(v32, v32)`**:
Directly invokes `foo@[I32, T(t2)]@(I32, I32)` using copies of v32 and v32.

**`foo(v64, v32)`**:

This call creates a conversion of the first argument to `I32` and then invokes `foo@[I32, T(t2)]@(I32, I32)` using the
converted value and the original `v32`.

**`foo(v32, v64)`**:

This does not perform any conversions and directly invokes `foo@[I32, T(t2)]@(I32, I64)` using the v32 and v64 values.

**`foo(v64, v64)`**:

This call creates a conversion of the first argument to `I32` and then invokes `foo@[I32, T(t2)]@(I32, I64)` using the
converted value and the original `v64`.

## callee_temploid_instanciation

Similar to callee_temploid_selection, but also applies the instanciation to the callee.