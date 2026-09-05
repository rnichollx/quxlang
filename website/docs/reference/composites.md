# Composites

A composite is an anonymous structural record. It has ordinary struct storage,
field construction, copying, moving, and destruction. Its type is determined
by its field names and declared field types.

For an introduction with option-forwarding examples, see the
[Composites Overview](../overview/composites.md).

## Literal syntax

```quxlang
VAR record AUTO := :{
  .a = 1 AS I32;
  .b = 9 AS I32;
  .nested = :{ .x = 3 AS I32; };
  .THIS = 5;
  .OTHER = 7;
};
VAR empty AUTO := :{};
ASSERT(record.nested.x == 3);
ASSERT(COMPOSITE_FIELD_COUNT(DECLTYPE(empty)) == 0);
```

Each field is `.name = expression`. Semicolons separate fields; a trailing
semicolon is optional. Field names must be unique. Ordinary identifiers and
the keyword names accepted for named call arguments, including `THIS`, `OTHER`,
`ARG`, and `RETURN`, are accepted. A valid field name can still be reserved
when the record is used in a call.

Initializer expressions are evaluated once, in source order. The record's
canonical field order is lexicographic by name. Access uses ordinary `.field`
syntax, including nested projections.

## Type identity and literal storage

Field types preserve initializer expression types, including references.
Two composites have the same type exactly when their field names and declared
field types match; initializer order is irrelevant:

```quxlang
VAR first AUTO := :{ .z = 9 AS I32; .a = 1 AS I32; };
VAR second AUTO := :{ .a = 4 AS I32; .z = 8 AS I32; };
ASSERT(SAME_TYPES(DECLTYPE(first), DECLTYPE(second)));
```

An expression naming an ordinary mutable local has type `MUT& T`, so storing it
directly creates a reference field. An expression producing a pure `T` creates
an owned field. Existing `CONST&`, `MUT&`, `TEMP&`, and `WRITE&` field types are
preserved.

Unconverted numeric and string literals retain their exact literal types.
They have no stored payload. Numeric literals with different values therefore
produce different field types:

```quxlang
VAR record AUTO := :{ .number = 42; .text = "example"; };
ASSERT(SAME_TYPES(COMPOSITE_FIELD_TYPE(DECLTYPE(record), "number"), TYPEOF(42)));
VAR number I32 := record.number;
ASSERT(number == 42);

STATIC_IF (ARCH_IS_LAYOUTLESS == FALSE)
{
  ASSERT(SIZEOF(TYPEOF(42)) == 0);
  ASSERT(SIZEOF(DECLTYPE(record)) == 0);
}
```

Use an explicit conversion such as `42 AS I32` to store an ordinary numeric
value. A field of that type can later hold a different `I32`. Zero-size literal
fields do not add payload bytes to a containing record; ordinary layout and
alignment rules still apply to its other fields. Layout queries are subject
to the target's [layout restrictions](toolchain/backends-and-layout.md).

## `APPLY`

The expression form is:

```text
APPLY argument_composite TO callable
```

The argument composite is evaluated first, then the callable. Each expression
is evaluated once. Top-level fields become named arguments with the same
names; nested composites remain individual argument values. The expression's
result is the result of the call.

```quxlang
::clamp FUNCTION(@value I32, @minimum I32 DEFAULT(0), @maximum I32 DEFAULT(100)): I32
{
  IF (value < minimum) { RETURN minimum; }
  IF (value > maximum) { RETURN maximum; }
  RETURN value;
}

VAR options AUTO := :{ .value = 150; .maximum = 80; };
ASSERT((APPLY options TO clamp) == 80);
```

Normal parameter adaptation, default arguments, and overload resolution apply.
Field projection uses the source's ordinary reference access. Use
`COMPOSITE_FORWARD` when owned values should be available as `TEMP&` arguments.

Functions, procedure pointers, lambdas, bound member functions, and objects
with `OPERATOR()` are supported. `APPLY` does not invoke constructors and does
not supply positional arguments. It rejects a non-composite argument, a
`RETURN` field, and an explicit `THIS` field when the callable already supplies
an implicit receiver. A free function may accept an explicitly declared
`@THIS` parameter. Parenthesize `APPLY` when using its result as an operand of
another expression.

## `KWARGS` parameters

The final parameter of a function can capture unmatched named arguments:

```text
@KWARGS ...
@KWARGS:local_name ...
```

There is at most one such parameter and it has no separate element type.
Explicit named parameters consume matching arguments first; the remaining
arguments form a composite bound to `KWARGS`, or to the given local alias.
The captured fields preserve the supplied argument types. An empty capture is
valid. `THIS` and `RETURN` are not captured as unmatched keyword arguments.

The composite is available as an ordinary value expression, supports
`DECLTYPE`, and can be passed to reflection, transforms, or `APPLY`. The
positional `PACK_*` operations do not inspect it.

This wrapper supplies a default only when the caller omitted that field:

```quxlang
::clamp_with_default_minimum FUNCTION(@KWARGS:options ...): I32
{
  STATIC_IF (COMPOSITE_CONTAINS(DECLTYPE(options), "minimum"))
  {
    RETURN APPLY COMPOSITE_FORWARD(options) TO clamp;
  }
  STATIC_ELSE
  {
    RETURN APPLY COMPOSITE_JOIN(
      COMPOSITE_FORWARD(options), :{ .minimum = 10; }
    ) TO clamp;
  }
}

ASSERT(clamp_with_default_minimum(@value 3) == 10);
ASSERT(clamp_with_default_minimum(@value 3, @minimum 1) == 3);
```

After ordinary overload ranking ties, a candidate without `KWARGS` is preferred
to one with it. Among remaining candidates, more explicitly matched named
parameters are preferred. Declaration order does not select the overload.
An uninstantiated `KWARGS` function cannot be converted directly to a procedure
pointer; a procedure pointer needs a concrete parameter interface.

## Static reflection

Use a composite type, commonly `DECLTYPE(record)`, for metadata queries. Type
queries accept a reference to a composite type as well. Field selectors must
be compile-time constants.

| Operation | Result and arguments |
| --- | --- |
| `COMPOSITE_CONTAINS(T, name)` | Boolean indicating whether composite type `T` has the compile-time string name. |
| `COMPOSITE_FIELD_COUNT(T)` | Compile-time numeric literal containing the number of fields. |
| `COMPOSITE_FIELD_NAME(T, index)` | String literal naming the field at the zero-based canonical index. |
| `COMPOSITE_FIELD_TYPE(T, selector)` | Declared field type, selected by a compile-time string name or unsigned index. |
| `COMPOSITE_FIELD_GET(value, selector)` | Ordinary field projection from a composite value, selected by name or index. |

Indices enumerate names lexicographically, independently of initializer order.
An unknown field or out-of-range index is an error; `COMPOSITE_CONTAINS` returns
false for an absent name. `COMPOSITE_FIELD_TYPE` reports the declared field
type, while `COMPOSITE_FIELD_GET` preserves normal access qualification:

```quxlang
VAR record AUTO := :{ .z = 9 AS I32; .a = 1 AS I32; };
ASSERT(COMPOSITE_CONTAINS(DECLTYPE(record), "a"));
ASSERT(SAME_TYPES(COMPOSITE_FIELD_TYPE(DECLTYPE(record), 0), I32));
ASSERT(SAME_TYPES(TYPEOF(COMPOSITE_FIELD_GET(record, "a")), MUT& I32));
ASSERT(COMPOSITE_FIELD_GET(record, 1) == 9);
```

Use `STATIC_WHILE` for indexed iteration over heterogeneous fields, as shown
in the [Overview](../overview/composites.md#inspect-fields-at-compile-time).

## Reference transforms

`COMPOSITE_TIE(source)` and `COMPOSITE_FORWARD(source)` evaluate `source` once
and return a new composite with the same names and reference fields.

| Declared source field | `COMPOSITE_TIE` | `COMPOSITE_FORWARD` |
| --- | --- | --- |
| Owned `T`, mutable source | `MUT& T` | `TEMP& T` |
| Owned `T`, constant source | `CONST& T` | `CONST& T` |
| Existing reference | Same reference type | Same reference type |

The transforms are shallow: an owned nested composite becomes a reference to
that composite. They require a readable source and do not copy or move its
owned fields. A `WRITE&` field already stored inside a readable composite keeps
its qualifier. Neither transform extends the lifetime of referenced objects.

```quxlang
VAR record AUTO := :{ .amount = 5 AS I32; };
VAR tied AUTO := COMPOSITE_TIE(record);
VAR forwarded AUTO := COMPOSITE_FORWARD(record);
ASSERT(SAME_TYPES(COMPOSITE_FIELD_TYPE(DECLTYPE(tied), "amount"), MUT& I32));
ASSERT(SAME_TYPES(COMPOSITE_FIELD_TYPE(DECLTYPE(forwarded), "amount"), TEMP& I32));
tied.amount := 13;
ASSERT(record.amount == 13);
```

## Field transforms

| Operation | Result |
| --- | --- |
| `COMPOSITE_JOIN(left, right)` | All fields from two composites with disjoint names. |
| `COMPOSITE_SELECT(source, name, ...)` | Only the named fields. |
| `COMPOSITE_EXCLUDE(source, name, ...)` | All fields except the named fields. |
| `COMPOSITE_SPLIT(source, name, ...)` | A composite with `.selected` and `.remainder` composites. |

Selection names are compile-time strings. Missing names and repeated selection
names are errors. Join rejects collisions instead of replacing an existing
field. With no names, select returns an empty composite, exclude retains all
fields, and split returns an empty `.selected` with all fields in `.remainder`.
Join evaluates its left source before its right source.

Transforms preserve the selected fields' declared types and use ordinary
construction. Owned fields copy from a readable lvalue when copying is
supported, or move from an expiring source when moving is supported. Existing
reference fields retain their bindings. Move-only fields require a suitable
source, for example `COMPOSITE_SELECT(MOVE(record), "item")`.

Splitting does not reassign the storage of the original record's subobjects.
To share fields across the results, split a tied or forwarded composite:

```quxlang
VAR record AUTO := :{ .a = 1 AS I32; .b = 2 AS I32; .c = 3 AS I32; };
VAR parts AUTO := COMPOSITE_SPLIT(COMPOSITE_TIE(record), "a", "c");
parts.selected.a := 19;
ASSERT(record.a == 19);
VAR joined AUTO := COMPOSITE_JOIN(parts.selected, parts.remainder);
ASSERT(joined.c<- == record.c<-);
```

See [References](references.md), [Move Semantics](move-semantics.md),
[Call Arguments](call-arguments.md), and [Variadic Packs](variadic-packs.md).
