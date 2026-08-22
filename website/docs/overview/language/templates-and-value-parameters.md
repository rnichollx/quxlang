# Overview of Templates

A template declaration introduces type or compile-time value parameters before
the declaration it controls.

## Type parameters

```quxlang
::box TEMPLATE(TYPE AUTO(t)) STRUCT
{
  .value VAR t;
}

VAR integer_box box#(I32);
```

`TYPE AUTO(t)` binds the supplied type to `t`. A named API parameter can be used
when a template has several arguments:

```quxlang
::typed_box TEMPLATE(@T TYPE AUTO(t)) STRUCT
{
  .value VAR t;
}

VAR value typed_box#(@T I32);
```

A named `TYPE` parameter may omit an explicit matching pattern. Give it a
lowercase local name when the body needs to name the bound type:

```quxlang
::direct_box TEMPLATE(@T:t TYPE) STRUCT
{
  .value VAR t;
}
```

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

- `name#(...)` supplies the full template argument list.
- Named arguments use `@name expression` inside that list.
- The compact `name#Type` spelling supplies the conventional `@T` type
  argument used by compiler and library templates.

For example:

```quxlang
VAR counter ATOMIC#U32;
```

Template instantiations are ordinary type symbols and may be nested inside
arrays, references, pointers, procedure types, and other instantiations.

See [Type queries and deduction](type-queries-and-deduction.md) and
[Variadic packs](variadic-packs.md).

## Reference

See the [Templates Reference](../../reference/templates-and-value-parameters.md) for the complete
language rules, constraints, and technical edge cases.
