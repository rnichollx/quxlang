# Overview of Public Field Reflection

`PUBLIC_FIELD_*` operations let compile-time code inspect the public fields
declared directly on a named `STRUCT` or `IBC_STRUCT`. You can count fields,
look up their names and types, and read or write a field selected at compile
time.

## Inspect public fields

```quxlang
::sample STRUCT
{
  .reading VAR I32;
  PRIVATE(CLASS) .cached VAR I32;
  .code VAR U64;
}

::inspect_sample STATIC_TEST
{
  ASSERT(PUBLIC_FIELD_COUNT(sample) == 2);
  ASSERT(PUBLIC_FIELD_CONTAINS(sample, "reading"));
  ASSERT(PUBLIC_FIELD_CONTAINS(sample, "cached") == FALSE);
  ASSERT(SAME_TYPES(TYPEOF(PUBLIC_FIELD_NAME(sample, 0)), TYPEOF("reading")));
  ASSERT(SAME_TYPES(PUBLIC_FIELD_TYPE(sample, "code"), U64));
}
```

Indices start at zero and follow declaration order. Here, `reading` is index
zero and `code` is index one, regardless of their physical layout.
`PUBLIC_FIELD_NAME` returns a string literal; the type comparison above checks
that its exact literal is `"reading"`.

Any field marked `PRIVATE(...)` is excluded, even inside a scope allowed to
access that field. Inherited fields, base selectors, member functions, and
static or nested declarations are also excluded. See [Privacy](privacy.md)
and [Inheritance](inheritance.md).

## Read and write a selected field

```quxlang
::access_sample STATIC_TEST
{
  VAR value sample;
  PUBLIC_FIELD_GET(value, "reading") := 23;
  ASSERT(PUBLIC_FIELD_GET(value, 0) == 23);
  ASSERT(value.reading == 23);
  ASSERT(PUBLIC_FIELD_COUNT(DECLTYPE(value)) == 2);

  VAR constant CONST& sample := value;
  ASSERT(SAME_TYPES(TYPEOF(PUBLIC_FIELD_GET(constant, "reading")), CONST& I32));
}
```

`PUBLIC_FIELD_TYPE` reports a field's declared type. `PUBLIC_FIELD_GET` behaves
like ordinary `.field` access, including its reference qualification. A
constant receiver therefore provides constant access to an owned field.

## Walk fields at compile time

Use `STATIC_VAR`, `STATIC_WHILE`, and `STATIC_EVAL` to select successive fields:

```quxlang
::pair STRUCT
{
  .z VAR I32;
  .a VAR I32;
}

::sum_pair_fields STATIC_TEST
{
  VAR value pair;
  value.z := 11;
  value.a := 17;
  VAR total I32 := 0;
  STATIC_VAR index SZ := 0;
  STATIC_WHILE (index < PUBLIC_FIELD_COUNT(pair))
  {
    total := total + PUBLIC_FIELD_GET(value, index);
    STATIC_EVAL index++;
  }
  ASSERT(total == 28);
}
```

Each expanded iteration has a compile-time field selector. The example sums
two `I32` fields; code visiting fields of different types must support each
selected type.

Use a type name or a bound type parameter in the loop condition. The current
`STATIC_WHILE` condition evaluation cannot resolve `DECLTYPE` of a runtime
local from the surrounding body. See
[Compile-Time Evaluation](compile-time-evaluation.md).

## Reference

The [Public Field Reflection Reference](../reference/public-field-reflection.md)
specifies selectors, supported types, filtering, and errors. Anonymous
[composites](composites.md#inspect-fields-at-compile-time) have their own
`COMPOSITE_*` reflection operations.
