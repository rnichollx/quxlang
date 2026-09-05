# Type Queries and Deduction

Quxlang exposes type identity, expression type, layout, and integer facts as
expressions. Most queries are evaluated at compile time; `DYNAMIC_TYPE_OF`
inspects an object:

```quxlang
::type_queries STATIC_TEST
{
  VAR value I32 := 12;

  ASSERT(SAME_TYPES(DECLTYPE(value), I32));
  ASSERT(SAME_TYPES(TYPEOF(value), MUT& I32));
  ASSERT(SIZEOF(I32) == 4);
  ASSERT(BITS(I32) == 32);
  ASSERT(IS_SIGNED(I32));
  ASSERT(IS_INTEGRAL(U32));
  ASSERT(BIT 5 == 32);
  ASSERT(TYPE_INDEX_OF(I32) != TYPE_INDEX_OF(I64));

  STATIC_IF(ARCH_IS_LAYOUTLESS == FALSE)
  {
    ASSERT(ALIGNOF(I32) == 4);
  }
}
```

## Type identity

- `DECLTYPE(symbol)` returns the declared value type.
- `TYPEOF(expression)` preserves the expression form, including reference
  qualification.
- `SAME_TYPES(left, right)` compares two types.
- `TYPE_INDEX_OF(T)` produces the stable program type identity used by
  `TYPE_INDEX` values.

## Polymorphism trait

```quxlang
TYPE_IS_POLYMORPHIC(T)
```

Returns a compile-time `BOOL`: `TRUE` for a concrete struct declared
`POLYMORPHIC` or `VIRTUAL_POLYMORPHIC`, and `FALSE` for other concrete types.
The category is sufficient even when the struct declares no virtual methods
or opts out of virtual destruction with `NONVIRTUAL`.

Aliases and concrete template instantiations are resolved before the test.
Pointers, references, arrays, interfaces, primitive types, and `VOID` return
`FALSE`; the query does not strip a reference or inspect a pointee or element.
Non-type arguments, such as variables and functions, are compilation errors.
Use a concrete type name or a resolved type argument; `THISTYPE` is not
currently resolved by this query.

The trait can be used in `STATIC_IF`, assertions, and declaration conditions
such as `ENABLE_IF`. It requires neither object construction nor runtime RTTI
and is available on both native and layoutless targets. See the
[Overview examples](../overview/type-queries-and-deduction.md#test-polymorphism).

## Dynamic type identity

```quxlang
DYNAMIC_TYPE_OF(ptr)
```

Returns a `TYPE_INDEX` for the active dynamic type of the pointed-to object.
Compare it with `TYPE_INDEX_OF(T)` to test exact type identity. A base pointer
to a complete derived object therefore reports the derived type, including
through secondary nonvirtual bases or shared virtual bases.

The operand must be a readable instance pointer to a concrete `POLYMORPHIC`
or `VIRTUAL_POLYMORPHIC` struct. Pointers to nonpolymorphic structs, primitive
pointers, and object references are compilation errors. The operand is
evaluated exactly once.

A null pointer causes undefined behavior; it does not return
`TYPE_INDEX_OF(VOID)`. Constant evaluation diagnoses a null operand.

During a constructor or destructor, the result is the type of the active
construction or destruction phase. A base constructor or destructor reports
that base type; during the complete object's steady lifetime it reports the
complete type.

`DYNAMIC_TYPE_OF` is available on native targets, including during constant
evaluation of valid objects. It is not yet implemented by the JVM backend.
See [Inheritance](inheritance.md) and the
[dynamic identity example](../overview/type-queries-and-deduction.md#identify-a-polymorphic-object).

## Public struct fields

Use [Public Field Reflection](public-field-reflection.md) to inspect the
fields of named structs. `PUBLIC_FIELD_COUNT`, `PUBLIC_FIELD_NAME`, and
`PUBLIC_FIELD_CONTAINS` inspect the public field list; `PUBLIC_FIELD_TYPE`
reports a declared field type, and `PUBLIC_FIELD_GET` projects a field from
an object. Public field indices follow declaration order.

## Composite metadata

`COMPOSITE_CONTAINS`, `COMPOSITE_FIELD_COUNT`, `COMPOSITE_FIELD_NAME`, and
`COMPOSITE_FIELD_TYPE` inspect a composite type. `COMPOSITE_FIELD_GET` projects
a field from a value. Names and indices are selected at compile time; indices
use canonical field-name order. See [Composite Reflection](composites.md#static-reflection)
for the exact signatures and access qualifiers.

## Layout and integer properties

- `SIZEOF(T)` gives byte size where the type has a known size.
- `ALIGNOF(T)` gives alignment and is unavailable for layoutless types.
- `BITS(T)` gives an integer type's bit width.
- `IS_SIGNED(T)` and `IS_INTEGRAL(T)` report integer properties.
- `TYPE_IS_LAYOUTLESS(T)` reports whether the current target omits a static byte
  layout for `T`.
- `BIT n` produces the compile-time numeric literal `2` raised to bit index
  `n`; cast it when a concrete runtime integer type is required.

An empty Quxlang struct has size zero and alignment one on targets with a
physical layout:

```quxlang
::empty STRUCT
{
}

::empty_layout STATIC_TEST
{
  STATIC_IF(ARCH_IS_LAYOUTLESS == FALSE)
  {
    ASSERT(SIZEOF(empty) == 0);
    ASSERT(ALIGNOF(empty) == 1);
  }
}
```

Quxlang does not add a synthetic byte to make distinct empty objects occupy
different addresses. Arrays and containing structs therefore preserve the
zero-sized placement of an empty element or field unless another field imposes
size or alignment. `SIZEOF` and `ALIGNOF` are unavailable when the target is
layoutless.

## Deduction patterns

`AUTO(t)` matches the non-reference form of the type presented to it. Reusing
the name `t` requires every occurrence to bind the same type:

```quxlang
::identity FUNCTION(@ARG:value AUTO(t)): t
{
  RETURN value;
}
```

`TT(t)` matches the presented type as-is, including a reference qualifier. It
is the direct type-template pattern used when reference identity itself must
participate in matching:

```quxlang
::accept_direct_type FUNCTION(@ARG:value TT(t)): BOOL
{
  RETURN TRUE;
}
```

For overload ranking, an exact non-template reference match ranks before a
direct `TT` template match. A direct `TT` match ranks before adding a constant
reference qualification.

`DECAY(t)` is the forwarding-oriented deduction pattern. It preserves mutable
and constant lvalue references but removes `TEMP&` from an expiring argument:

```quxlang
::decay_value FUNCTION(@ARG:value AUTO& DECAY(result)): result
{
  RETURN FORWARD(value);
}
```

Use `AUTO(t)` to deduce the non-reference value type, `TT(t)` to retain the
presented reference shape, and `DECAY(t)` when an expiring reference should
become an owned value type while lvalue references remain references.

See [Templates](templates-and-value-parameters.md),
[Variadic packs](variadic-packs.md), and
[Availability and targets](availability-and-targets.md).
