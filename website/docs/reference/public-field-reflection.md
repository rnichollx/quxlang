# Public Field Reflection

The `PUBLIC_FIELD_*` intrinsics expose the directly declared public instance
fields of named `STRUCT` and `IBC_STRUCT` types. These are compile-time
metadata and selection operations; they do not provide a runtime reflection
registry.

For usage examples and indexed iteration, see the
[Public Field Reflection Overview](../overview/public-field-reflection.md).

## Operations

| Operation | Result |
| --- | --- |
| `PUBLIC_FIELD_COUNT(T)` | Numeric literal containing the number of direct public fields. |
| `PUBLIC_FIELD_NAME(T, index)` | String literal naming the field at a zero-based declaration index. |
| `PUBLIC_FIELD_CONTAINS(T, name)` | Boolean indicating whether a direct public field has the given compile-time string name. |
| `PUBLIC_FIELD_TYPE(T, selector)` | Declared type of the selected field. |
| `PUBLIC_FIELD_GET(value, selector)` | Ordinary member projection from `value` to the selected field. |

`T` is type syntax. Aliases and instantiated struct templates are supported.
References to a supported type are accepted by metadata operations; reference
qualification does not change the public field list or the declared field
types. Pointers are not implicitly dereferenced.

```quxlang
::holder TEMPLATE(@T TYPE) STRUCT
{
  .value VAR T;
  PRIVATE(CLASS) .hidden VAR T;
}

::integer_holder ALIAS holder#(I64);
::element_type ALIAS PUBLIC_FIELD_TYPE(integer_holder, "value");

::inspect_holder STATIC_TEST
{
  ASSERT(PUBLIC_FIELD_COUNT(CONST& integer_holder) == 1);
  ASSERT(SAME_TYPES(element_type, I64));
  ASSERT(SAME_TYPES(PUBLIC_FIELD_TYPE(holder#(I32), 0), I32));
}
```

Primitive types, pointers, arrays, unions, variants, interfaces, and anonymous
composites are not supported subjects. Use the existing
[composite reflection operations](composites.md#static-reflection) for
composites; their canonical indices remain lexicographic.

## Field membership and order

All five operations use the same field list:

1. Start with active instance variable declarations directly on the type,
   after `INCLUDE_IF` filtering.
2. Exclude every field carrying a `PRIVATE(...)` annotation, whether attached
   individually or through a `PRIVATE` block.
3. Preserve the remaining fields' declaration order and number them from zero.

The list excludes inherited fields, named and anonymous base subobjects,
member functions, and static or nested declarations. A derived type with no
direct public fields has a count of zero even if its base types have fields.

The result does not depend on the calling scope's privileges. Class-private,
module-private, and named-scope-private fields remain excluded when the caller
can otherwise access them. Ordinary [privacy](privacy.md) rules still govern
access to the subject type.

The order is independent of byte offsets and layout reordering, including for
`IBC_STRUCT`. Inactive or private declarations consume no public field index.
An empty or private-only struct has a count of zero.

Counting, naming, and testing membership do not require object construction,
physical layout, or resolution of every field's type. `PUBLIC_FIELD_TYPE`
resolves the selected declaration's type in its owning struct's context,
including template bindings.

## Selectors and errors

`PUBLIC_FIELD_TYPE` and `PUBLIC_FIELD_GET` accept a compile-time string name or
an unsigned integer index. `PUBLIC_FIELD_NAME` accepts an unsigned index;
`PUBLIC_FIELD_CONTAINS` accepts a string name. String literals and compile-time
`STRING_CONSTANT` values are supported as names.

- A private, inherited, inactive, or absent name produces `FALSE` from
  `PUBLIC_FIELD_CONTAINS`.
- Selecting such a name with `PUBLIC_FIELD_TYPE` or `PUBLIC_FIELD_GET` is a
  compilation error.
- A negative index, an index greater than or equal to the public field count,
  a selector of the wrong type, or a nonconstant selector is a compilation
  error.
- An unsupported subject type is a compilation error, including for count and
  membership operations; it does not produce an empty list.

Selectors are evaluated in a compile-time context. Use a named type or bound
type parameter in a `STATIC_WHILE` condition or a nested metadata selector
such as `PUBLIC_FIELD_NAME(T, index)`. These separately evaluated expressions
cannot currently resolve `DECLTYPE` of a surrounding runtime local. Direct
metadata expressions such as `PUBLIC_FIELD_COUNT(DECLTYPE(value))` are
supported in an ordinary function or test body.

## Declared types and value access

`PUBLIC_FIELD_TYPE` preserves the declared field type, including a reference
field's qualifier. It does not apply the receiver's access qualifier.
`PUBLIC_FIELD_GET` evaluates its source once and follows ordinary `.field`
access, including reference-member behavior, access qualification, and
temporary lifetimes. It does not extend the lifetime of a temporary source.

```quxlang
::record STRUCT
{
  .number VAR I32;
}

::inspect_access STATIC_TEST
{
  VAR value record;
  PUBLIC_FIELD_GET(value, "number") := 19;
  ASSERT(PUBLIC_FIELD_GET(value, 0) == 19);
  ASSERT(SAME_TYPES(PUBLIC_FIELD_TYPE(CONST& record, 0), I32));
  ASSERT(SAME_TYPES(TYPEOF(PUBLIC_FIELD_GET(value, 0)), MUT& I32));

  VAR constant CONST& record := value;
  ASSERT(SAME_TYPES(TYPEOF(PUBLIC_FIELD_GET(constant, 0)), CONST& I32));
}
```

See [Structures](structs-and-members.md), [References](references.md),
[Type Queries](type-queries-and-deduction.md), and
[Compile-Time Evaluation](compile-time-evaluation.md).
