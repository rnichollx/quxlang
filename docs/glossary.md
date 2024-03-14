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
CLASS and FUNCTION declarations are incompatible.

In general, a FUNCTION can be fused with other FUNCTIONs, a TEMPLATE can be 
fused with other TEMPLATEs, and NAMESPACEs can be fused with other 
NAMESPACEs. CLASS and VAR declarations cannot be fused.

## Declaroid

A declaroid is anything that can be declared at class scope, like a variable
or a
function.
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

See also: Aggrgate-fusion, Temploid, Templexoid, Function, Variable, Class.

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

## Functoid

A functoid is any function-like expression that can be called with `OPERATOR()`.

## Function

A function is a declaration in the abstract syntax tree using the FUNCTION
keyword.

## Function Overload Set

In languages like C++, "function overload set" is the term used to describe
a collection of various function overloads that share the same name. In Qux,
an analogous concept is the "functum".
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

A temploid is something that can be instantiated. Functions and templates
are temploids.

## Temploidic-Function

A temploidic-function is a function where at least one of the parameters is
a _tempar_.

## Concrete-Function

A concrete-function is a function where all of the parameters are
concrete-types and no parameter is a tempar.

## Tempar

A tempar, (portmanteau of "template parameter"), is a function parameter
typoid which
performs tempar-matching when the function is instantiated.

Example:

```
FUNCTION(%a I32, %b T(t)) -> I32
{
  ...
}
```

Here, the type of the `%b` parameter is a tempar.

See also: Typoid

## Typoid

A typoid is a type-expression that can be used as a temploid parameter or
variable declaration. Typoids include concrete-types and tempars.

See also: Concrete-Type, Tempar

## Instantiation-Reference

An instantiation-reference is a reference to the instantiation of a
particular temploid,
or if it is unambiguous, a templexoid.

Example:

```
foo@(I32, I64)
```

Here, `foo` must be either a temploid or a templexoid where exactly 1 of the
temploids comprising the templexoid is a candidate for instantiation.

If there are multiple candidates, then the instantiation-reference is
proceded by a selection-reference in canonical form. Example:

```
foo@[I32, T(t)]@(I32, I64)
```

## Selection-Reference

A selection-reference is a reference to a specific temploid of a templexoid.

Example:

```
foo@[I32, T(t)]
```

Here, `foo` is a templexoid, and the result of the type-expression is a
non-instanciated temploid.