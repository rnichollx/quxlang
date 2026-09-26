# Overview of Type Queries

Type queries let compile-time code inspect an expression's type, compare types,
and ask about physical layout or integer properties. Deduction patterns let a
generic function bind a type from its arguments.

## Type inspection

```quxlang
::type_queries STATIC_TEST
{
  VAR value I32 := 12;

  ASSERT(SAME_TYPES(DECLTYPE(value), I32));
  ASSERT(SAME_TYPES(TYPEOF(value), MUT& I32));
  ASSERT(SIZEOF(I32) == 4);
  ASSERT(BITS(I32) == 32);
  ASSERT(IS_SIGNED(I32));
}
```

`DECLTYPE` reports the declared value type. `TYPEOF` preserves the expression's
reference qualification. `SIZEOF`, `ALIGNOF`, and `BITS` report layout and
integer facts where the target provides them.

An empty Quxlang structure has size zero and alignment one on a physical-layout
target:

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

An `ARCH_IS_LAYOUTLESS == FALSE` condition restricts layout queries to targets
that define object layout.

## Polymorphism queries

`TYPE_IS_POLYMORPHIC(T)` returns a compile-time `BOOL`. Both polymorphic struct
categories return `TRUE`; other concrete types return `FALSE`:

```quxlang
::message STRUCT POLYMORPHIC {}
::shared_message STRUCT VIRTUAL_POLYMORPHIC {}
::plain_message STRUCT {}

::polymorphism_queries STATIC_TEST
{
  ASSERT(TYPE_IS_POLYMORPHIC(message));
  ASSERT(TYPE_IS_POLYMORPHIC(shared_message));
  ASSERT(TYPE_IS_POLYMORPHIC(plain_message) == FALSE);
  ASSERT(TYPE_IS_POLYMORPHIC(I32) == FALSE);
  ASSERT(TYPE_IS_POLYMORPHIC(CONST->message) == FALSE);
}
```

The query tests its type operand directly; the query does not follow a pointer
to the pointee type. `TYPE_IS_POLYMORPHIC` does not construct an object and is
available on layoutless targets.

## Runtime type identification

`TYPE_INDEX_OF(T)` identifies a statically named type. `DYNAMIC_TYPE_OF(ptr)`
identifies the complete polymorphic object through a base pointer:

```quxlang
::timed_message STRUCT POLYMORPHIC
{
  .base_part BASE message;
  .timestamp VAR I64;
}

::dynamic_identity STATIC_TEST
{
  VAR object timed_message;
  VAR pointer CONST->message := object<-;
  ASSERT(DYNAMIC_TYPE_OF(pointer) == TYPE_INDEX_OF(timed_message));
}
```

This example requires a native target. `DYNAMIC_TYPE_OF` requires a readable
pointer to a polymorphic struct; using a nonpolymorphic struct pointer does not
compile. A null pointer causes undefined behavior. During construction and
destruction, the result follows the active constructor or destructor type.

## Argument type deduction

```quxlang
::identity FUNCTION(@value AUTO(t)): t
{
  RETURN value;
}
```

`AUTO(t)` binds the non-reference value type. `TT(t)` retains the presented
type, including a reference. `DECAY(t)` is useful for forwarding: it preserves
lvalue references but turns an expiring `TEMP&` into an owned value type.

Reusing a deduction name requires every occurrence to agree on the same bound
type.

## Public struct fields

[Public Field Reflection](public-field-reflection.md) inspects the fields of
named structs. `PUBLIC_FIELD_COUNT`, `PUBLIC_FIELD_NAME`, and
`PUBLIC_FIELD_CONTAINS` inspect the public field list; `PUBLIC_FIELD_TYPE`
reports a declared field type, and `PUBLIC_FIELD_GET` projects a field from
an object. Public field indices follow declaration order.

## Composite reflection

[Composites](../reference/composites.md#static-reflection) expose field counts,
names, declared types, and field projections through `COMPOSITE_*` operations.
`DECLTYPE(record)` supplies the type operand for metadata queries.
`COMPOSITE_FIELD_GET` accesses a field selected at compile time.

## Reference

See the [Type Queries and Deduction Reference](../reference/type-queries-and-deduction.md)
for every query, layoutless behavior, exact deduction transformations, and
overload-ranking relationships.
