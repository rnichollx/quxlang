# Templates

`TEMPLATE` parameterizes the declaration that immediately follows it. Template
parameters accept either types or compile-time values, and an instantiation
supplies arguments with `#(...)`.

## Declaration form

```text
TEMPLATE(parameter, ...) declaration
```

The controlled declaration may be a structure, function, variable, or another
declaration accepted in that context:

```quxlang
::box TEMPLATE(TYPE AUTO(element_type)) STRUCT
{
  .value VAR element_type;
}

::identity TEMPLATE(TYPE AUTO(value_type))
  FUNCTION(@value value_type): value_type
{
  RETURN value;
}
```

The template name denotes the parameterized declaration. A concrete type,
function, or object is selected only after argument binding succeeds.

## Positional type parameters

`TYPE` introduces a type parameter. Its following type pattern both constrains
and may bind the supplied type:

```quxlang
::box TEMPLATE(TYPE AUTO(element_type)) STRUCT
{
  .value VAR element_type;
}

VAR integer_box box#(I32);
```

`AUTO(element_type)` accepts the concrete type and binds it under the local name
`element_type`. A more specific type pattern can restrict the acceptable shape
while binding its deduced part, using the same deduction rules as function
parameters.

Positional parameters are supplied in source order. They have no call-site API
name.

## Named type parameters

A leading `@` gives a parameter a public template-argument name:

```quxlang
::pair TEMPLATE(@LEFT TYPE AUTO(left_type),
                @RIGHT TYPE AUTO(right_type)) STRUCT
{
  .left VAR left_type;
  .right VAR right_type;
}

VAR value pair#(@LEFT I32, @RIGHT F64);
```

`@API:local` separates the argument name from the name used in the declaration:

```quxlang
::direct_box TEMPLATE(@T:element_type TYPE) STRUCT
{
  .value VAR element_type;
}
```

For a named `TYPE` parameter, omitting the explicit pattern uses the parameter's
local name as its type binding. If no separate local name is present, the API
name is also the binding name. Named template parameter names must be unique.

## Value parameters

`VALUE Type` requires a compile-time value compatible with `Type`:

```quxlang
::fixed_buffer TEMPLATE(@count:element_count VALUE U64) STRUCT
{
  .values VAR [element_count]BYTE;
}

VAR packet fixed_buffer#(@count 32);
```

The value participates in the identity of the instantiation and may be used
wherever a static value of its type is accepted, including array bounds,
`STATIC_IF`, and other template arguments. A runtime expression cannot satisfy
a `VALUE` parameter.

Type and value parameters may be mixed in one list:

```quxlang
::array_box TEMPLATE(@T:element_type TYPE,
                     @count:element_count VALUE U64) STRUCT
{
  .items VAR [element_count]element_type;
}

VAR samples array_box#(@T F32, @count 16);
```

## Argument syntax

The full form is `name#(...)`:

```quxlang
VAR positional box#(I32);
VAR named direct_box#(@T I32);
```

Arguments use the same named-versus-positional grouping rules as calls. Every
required template parameter must receive one argument, no named parameter may
receive more than one, and an unexpected name or excess positional argument is
rejected.

`name#Type` is a compact form for one type argument named `@T`:

```quxlang
VAR counter ATOMIC#U32;
```

It is equivalent to `ATOMIC#(@T U32)`, not to an arbitrary first positional
argument. Use `#(...)` when the template does not expose `@T` or needs more than
one argument.

`name#[IndexType:ValueType]` is a type-only compact form for the conventional
named pair `@INDEX` and `@VALUE`:

```quxlang
::mapping TEMPLATE(@INDEX TYPE AUTO(index_type),
                   @VALUE TYPE AUTO(value_type)) STRUCT
{
  // ...
}

VAR scores mapping#[std::string:I32];
```

This is exactly equivalent to
`mapping#(@INDEX std::string, @VALUE I32)`. Both sides of `:` must be type
symbols. The shorthand does not bind positional parameters or differently
named parameters, and it may be nested wherever a type symbol is accepted.

## Instantiation and nested names

An instantiated declaration is an ordinary symbol of its resulting kind. A
class instantiation may be used in arrays, pointers, references, procedure
types, and further instantiations, and its nested declarations are selected
from the instantiated symbol:

```quxlang
VAR values [4]box#(I32);
VAR pointer ->box#(I32);
```

The same declaration and equivalent argument set identify the same template
instantiation. Distinct type or value arguments produce distinct instantiated
symbols.

Function templates participate in overload resolution after their parameters
are deduced or explicitly supplied. A candidate whose template arguments cannot
bind is not callable. `ENABLE_IF` can then remove a successfully bound concrete
candidate; see [Overload Resolution](overload-resolution.md).

See [Type Queries and Deduction](type-queries-and-deduction.md) for `AUTO`
bindings and [Variadic Packs](variadic-packs.md) for pack parameters and pack
introspection.
