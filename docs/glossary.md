# Glossary

## Aggregate-Fusion

Aggregate-fusion is a stage in the compilation process when processing 
outer-scopes. It consists of combining multiple declarations of the same 
name into one. For example, the combination of multiple functions into a 
single functum. Aggregate-fusion can fail, for example:

```
::foo FUNCTION(%a I32):I32
{
  ...
}

::foo CLASS
{
  ...
}
```

In the above example, the aggregate-fusion of `::foo` will fail because the 
CLASS and FUNCTION declarations are incompatible. Variables and classes also 
cannot be fused, even with other variables and classes.

In general, a FUNCTION can be fused with other FUNCTIONs, a TEMPLATE can be 
fused with other TEMPLATEs, and NAMESPACEs can be fused with other 
NAMESPACEs. Unlike FUNCTION, NAMESPACE, and TEMPLATE declarations, CLASS and
VAR declarations cannot be fused. (i.e., there must be no more than one VAR
or CLASS with the same name in a given scope).

## Declaroid

A declaroid is anything that can be *declared*, like a variable
or a function.

The class of declaraoids does not include aggregates like templexoids, but
does include the individual teploids that comprise a templexoid. It also
includes non-temploid declaroids like variables and classes.

Example:

```
::foo FUNCTION(%a I32):I32
{
  ...
}

::bar VAR I32

::baz CLASS
{
  ...
}

::bif FUNCTION(%a I32):I32
{
  ...
}

::bif FUNCTION(%a I32, %b I32):I32
{
  ...
}
```

In the above example, the declarations of `::foo`, `::bar`, and `::baz`, are
all
declaroids. The two
`::bif` declarations are both individually declaroids, but the aggregate
`::bif` is
not a declaroid as a whole. Declaroids have a single abstract-syntax-tree 
representation. A declaroid either is directly defined in source code, or 
results from macro-execution prior to aggregate-fusion.

See also: Aggregate-fusion, Temploid, Templexoid, Function, Variable, Class.

## Declared Parameters

Declared parameters are the parameters of a temploid (function or template) as
they are declared in the source code.

Declared parameters are contrasted with formal parameters, which are the
parameters of a temploid after type decontextualization and dealiasing.

For example in the following code:

```

::baz CLASS
{
   ::bar CLASS 
   {
      ...
   }
   
   ::foo FUNCTION(%x bar)
   {
     ...
   }
}
```

The "declared parameters" of `::foo FUNCTION(%x bar)` are `(%x bar)`, whereas the formal parameters are `(%x [module]::baz::bar)`. 

### Differs From

*Paratype* - The paratype is the type of the positional and named parameters, however it does not include the parameter names. 
For example, the above formal paratype would be `([module]::baz::bar)` and the declared paratype would be `(bar)`.

*Signature* - The signature is a combination of the paratype and the return type. For example, the above formal signature 
would be `([module]::baz::bar): I32` and the declared signature would be `(bar): I32`.

## Functanoid

A functanoid is a function which is instantiated with a set of parameter
types. A functanoid differs from a procedure in that functanoids are
abstract compile-time concepts, and there exists a single functanoid which can
correspond to
multiple
procedures for different architectural subsets of the target machine.
Functanoids do
not occupy
memory at
runtime, unlike a procedure, which is an executable object consisting of a
series of machine instructions.

## Formal Type

A formal type is the type of a variable or parameter after resolving symbols into their globally
unique, context-free identifiers. For example, a type `foo` might be resolved to `[mymodule]::bar::foo`.
However, the formal type does not instantiate temploidic parameters. For instance, a formal type
might include temploidic types like `T(t)` (any type), `PTR(p)` (any pointer type), or `INT(i)`
(any size integer).


## Formal Parameters

Formal parameters are the parameters of a temploid (function or template) after
type decontextualization and dealiasing. (e.g. resolving `(%x foo, %y myint32alias)`
to `(%x [mymodule]::baz::foo, %y I32)`)

Formal parameters are contrasted with declared parameters, which are the parameters of a temploid
as they are declared in the source code.

The formal parameters do not include the instanciation of parameters, so for example, a declared type
of T(t) would remain as T(t) in the formal parameters, but would be resolved to a concrete type in the
instantiated parameters.


## Functoid

A functoid is any function-like expression that can be called with `OPERATOR()`.

## Function

A function is a declaration in the abstract syntax tree using the FUNCTION
keyword.

## Function Overload Set

In languages like C++, "function overload set" is the term used to describe
a collection of various function overloads that share the same name. In Qux,
a roughly analogous concept is the "functum", although these may not be
equivalent in all cases.
See also: Functum.

## Function-Pointer

A function-pointer is a concept in some other programming languages like C
and C++.
Because
Qux functions are like "template functions" in other programming languages like
C++,
Qux
does not have a "function-pointer", but it does have the
concept of a procedure-pointer, which is roughly analogous to a
function-pointer in C and C++.

## Functum

A functum is a collection of 1 or more functions that share a symbolic name.
A functum is a type of templexoid.

## Template

A template is a declaration that takes a set of parameters and produces a
declaroid with substituted parameters.

## Symboid

A symboid is anything that can always be referred to by a symbolic name. This
includes templexoids and non-temploid declaroids.

## Templex

A templex is a collection of 1 or more templates that share a symbolic name.

## Templexoid

A templexoid is a collection of multiple temploids that share a symbolic
name. I.e. either a templex or functum.

## Temploid

A temploid is an individual declaration of something that can be instantiated. Individual function and templates
declarations are temploids.

### Differs from

* Templexoid* - A templexoid is a collection of 1 or more temploids that share a symbolic name.

## Temploidic-Function

A temploidic-function is a function where at least one of the parameters is
a _tempar_.

## Concrete-Function

A concrete-function is a function where all of the parameters are
concrete-types and no parameter is a tempar.

## Tempar

A tempar, (portmanteau of "template parameter"), is a part of a formal temploidic type that is parameterized
when the temploid is instantiated.

Example:

```
FUNCTION(%a I32, %b T(t)) -> I32
{
  ...
}
```

Here, the formal type of the `%b` parameter is the temploidic type `T(t)`, where `T` is the generic "any type" keyword 
and `t` is a tempar. When the function is instantiated, `t` will be parameterized with a type and the `t`
tempar will become an alias for that concrete type. Note that `T(t)` is a temploidic type, not a tempar, but `t` itself
is a tempar within this context.

See also: Type

## Type

A type is a symbol that can be used as a temploid parameter, function return type, or
in a variable declaration. Types include concrete-types and temploidic types.

See also: Concrete-Type, Tempar

### See also

 * Concrete Type
 * Temploidic Type
 * Formal Type
 * Declared Type
 * Instantiated Type
 * Paratype

## Instantiation-Reference

An instantiation-reference is a reference to the instantiation of a
particular temploid or templexoid with a given set of parameters.

Example:

```
foo#(I32, I64)
```

If there are multiple candidates, then the instantiation-reference is
proceded by a selection-reference in canonical form. Example:

```
foo#[I32, T(t)]#(I32, I64)
```

This can be shorted to the selstantiation syntax sugar:

```
foo#{I32, T(t): I64}
```

## Selection-Reference

A selection-reference is a reference to a specific temploid of a templexoid.

Example:

```
foo#[I32, T(t)]
```

Here, `foo` is a templexoid, and the result of the type-expression is a
non-instanciated temploid.