# Templates

The `TEMPLATE` keyword parameterizes the declaration that immediately follows it. Template
parameters accept either types or static values. A template can be instantiated with arguments
using `#(...)` postfix syntax.

## Declaration form

```text
TEMPLATE(parameter, ...) declaration
```

The controlled declaration may be a structure, function, variable, or another
declaration accepted in that context:

```quxlang
::box TEMPLATE(@T CLASS) STRUCT
{
  .value VAR T;
}

::identity TEMPLATE(@T TYPE)
  FUNCTION(@value T): T
{
  RETURN value;
}
```

The template name denotes the parameterized declaration. A concrete type,
function, or object is selected only after argument binding succeeds.

## Named type parameters

A leading `@` gives a parameter a public template-argument name:

```quxlang
::pair TEMPLATE(@LEFT CLASS, @RIGHT CLASS) STRUCT
{
  .left VAR LEFT;
  .right VAR RIGHT;
}

VAR value pair#(@LEFT I32, @RIGHT F64);
```

The API name is also the template-body binding name unless `:` supplies a
different local name:

```quxlang
::box TEMPLATE(@element:element_type CLASS) STRUCT
{
  .value VAR element_type;
}
```

This alias form is for an actual naming distinction; `@T CLASS` already binds
the supplied type as `T` and does not need `@T:t` or `AUTO(t)`.

There are three type-parameter forms:

- `@T TYPE` accepts and exactly binds any type, including references. It is
  equivalent to `@T TYPE TT`.
- `@T CLASS` accepts any non-reference type. It is equivalent to
  `@T TYPE AUTO`.
- `@T TYPE TypePattern` accepts types matching the explicit pattern.

These rules apply to template parameters. An `AUTO` function parameter uses
function argument adaptation and may accept a reference argument; it is not a
`CLASS` template constraint.

The declared name binds the entire type matched by the type pattern. Given:

```quxlang
@arg TYPE MUT& AUTO
```

an argument of type `MUT& I32` binds `arg` to `MUT& I32`. `AUTO` is a wildcard
and needs no capture name. If the declaration instead reads:

```quxlang
@arg TYPE MUT& AUTO(element_type)
```

`arg` binds the entire `MUT& I32` type, while `element_type` is an
additional temploidic capture bound to the nested `I32` type. Repeated explicit
captures use the template-deduction consistency rules.

Template bindings such as `T`, `arg`, and `element_type` are valid type
expressions wherever their bound types are accepted.

## Positional type parameters

Positional template parameters begin with `%`, but still require a local
binding name:

```quxlang
::either TEMPLATE(%left_type CLASS, %right_type CLASS) STRUCT
{
  .left VAR left_type;
  .right VAR right_type;
}

VAR value either#(% [I32, F64]);
```

They are supplied in source order through an explicit `% [...]` group and do
not expose call-site API names.

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
::array_box TEMPLATE(@T CLASS,
                     @count:element_count VALUE U64) STRUCT
{
  .items VAR [element_count]T;
}

VAR samples array_box#(@T F32, @count 16);
```

## Argument syntax

Template argument lists use the same named and positional grouping syntax as
function calls:

```quxlang
VAR named pair#(@LEFT I32, @RIGHT F64);
VAR positional either#(% [I32, F64]);
```

Every required template parameter must receive one argument, no named
parameter may receive more than one, and an unexpected name or excess
positional argument is rejected. Multiple positional groups form one logical
positional sequence, as they do in function calls.

There is one template-specific bare-argument convention. A list containing one
unprefixed expression binds it to the named parameter `@T`:

```quxlang
VAR integer_box box#(I32);
```

This is equivalent to `box#(@T I32)`, not to supplying the first positional
parameter. More than one bare expression is invalid; use named arguments or
`% [...]` as appropriate. In ordinary function calls, the corresponding bare
argument convention binds `@ARG` instead.

`name#Type` is a compact form for the same `@T` type argument:

```quxlang
VAR counter ATOMIC#U32;
```

`name#[index_type:value_type]` is a compact form for the
named pair `@INDEX` and `@VALUE`:

```quxlang
::mapping TEMPLATE(@INDEX CLASS, @VALUE CLASS) STRUCT
{
  // ...
}

VAR scores mapping#[std::string:I32];
```

This is exactly equivalent to
`mapping#(@INDEX std::string, @VALUE I32)`. Both sides of `:` must be type
symbols.

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

See [Call Arguments](call-arguments.md) for the shared grouping grammar,
[Type Queries and Deduction](type-queries-and-deduction.md) for temploidic
captures, and [Variadic Packs](variadic-packs.md) for pack parameters and pack
introspection.
