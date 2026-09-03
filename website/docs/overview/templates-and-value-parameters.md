# Overview of Templates

A template declaration introduces named type or compile-time value parameters
before the declaration it controls. Every template parameter has a binding
name.

## Type parameters

The conventional single-type template exposes `@T`:

```quxlang
::box TEMPLATE(@T CLASS) STRUCT
{
  .value VAR T;
}

VAR integer_box box#(I32);
```

`CLASS` accepts any non-reference type and is equivalent to `TYPE AUTO`.
`TYPE` without a following pattern accepts any exact type, including a
reference, and is equivalent to `TYPE TT`. The supplied type is bound to `T`,
so the template body can use `T` directly as a type expression. The bare
argument in `box#(I32)` is the template shorthand for `@T I32`.

Templates with several public arguments give each one a descriptive name:

```quxlang
::pair TEMPLATE(@LEFT CLASS, @RIGHT CLASS) STRUCT
{
  .left VAR LEFT;
  .right VAR RIGHT;
}

VAR value pair#(@LEFT I32, @RIGHT F64);
```

Use `@API:local_name` only when the public argument name and the name used by
the template body genuinely differ:

```quxlang
::typed_box TEMPLATE(@element:element_type CLASS) STRUCT
{
  .value VAR element_type;
}
```

The named parameter binds the entire supplied type. For example,
`@arg TYPE MUT& AUTO` binds `arg` to `MUT& I32` when that is the supplied type.
Writing `MUT& AUTO(element_type)` additionally captures the nested `I32` as
`element_type`; it does not replace the whole-type `arg` binding.

## Value parameters

```quxlang
::fixed_box TEMPLATE(@count:element_count VALUE U64) STRUCT
{
  .values VAR [element_count]I32;
}

VAR four_values fixed_box#(@count 4);
```

`VALUE U64` requires a compile-time value of the specified type.

## Template argument syntax

Template arguments use the same explicit grouping syntax as call arguments:

- Named arguments use `@name expression`.
- Deliberately positional parameters are declared with `%name` and supplied
  inside `% [...]`.
- One bare argument in `name#(Type)` binds the conventional named parameter
  `@T`; it does not bind the first positional parameter.
- The compact `name#Type` spelling also supplies `@T`.
- The compact `name#[IndexType:ValueType]` spelling supplies the conventional
  `@INDEX` and `@VALUE` type arguments used by associative containers.

An intentionally positional template therefore writes both sides explicitly:

```quxlang
::either TEMPLATE(%left_type CLASS, %right_type CLASS) STRUCT
{
  .left VAR left_type;
  .right VAR right_type;
}

VAR value either#(% [I32, F64]);
```

`TEMPLATE(CLASS)`, `TEMPLATE(TYPE)`, and `TEMPLATE(% CLASS)` are invalid because
they do not declare a binding name.

For example:

```quxlang
VAR counter ATOMIC#U32;
VAR scores std::map#[std::string:I32];
```

The map spelling is equivalent to
`std::map#(@INDEX std::string, @VALUE I32)`.

Template instantiations are ordinary type symbols and may be nested inside
arrays, references, pointers, procedure types, and other instantiations.

See [Call arguments](call-arguments.md),
[Type queries and deduction](type-queries-and-deduction.md), and
[Variadic packs](variadic-packs.md).

## Reference

See the
[Templates Reference](../reference/templates-and-value-parameters.md) for the
complete language rules, constraints, and technical edge cases.
