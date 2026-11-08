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
  ASSERT(BITS(I32) == 32);
  ASSERT(IS_SIGNED(I32));
  ASSERT(IS_INTEGRAL(U32));
  ASSERT(BIT 5 == 32);
  ASSERT(TYPE_INDEX_OF(I32) != TYPE_INDEX_OF(I64));

  STATIC_IF(ARCH_IS_LAYOUTLESS == FALSE)
  {
    VAR integer_size SZ := SIZEOF(I32);
    VAR integer_alignment SZ := ALIGNOF(I32);
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

## Function callability

`IS_FUNCTUM_CALLABLE_WITH(binding, @name Type, % [Type, ...])` produces a constant
`BOOL` indicating whether invocation lookup selects a function for the supplied
argument types. The binding names a functum or a coroutine descriptor. Function
templates require explicit template specialization before this query.

```quxlang
::increment FUNCTION(@value I32, @amount I32 DEFAULT(1)): I32
{
  RETURN value + amount;
}

::callability STATIC_TEST
{
  TEST_ASSERT(IS_FUNCTUM_CALLABLE_WITH(increment, @value I32));
  TEST_ASSERT(IS_FUNCTUM_CALLABLE_WITH(increment, @missing I32) == FALSE);
}
```

Named and positional arguments follow function-specialization argument syntax
and must name types. `COMPOSITE_UNPACK(CompositeType)` supplies field types as
arguments. An omitted argument list tests a zero-argument invocation. Member
functions require an explicit `@THIS` type, and coroutine candidates require an
explicit `@CONTROLLER` type. `@CONTROLLER AUTO` permits selection of a partial
coroutine descriptor.

Lookup applies ordinary argument adaptation, default parameters, declaration
constraints, and access control. No viable function produces `FALSE`. Ambiguous
selection, inaccessible selected declarations, and malformed arguments produce
compilation errors. The query does not execute a function or evaluate default
argument values. Successful selection does not validate execution of the selected
function body.

`IS_COROUTINE_CALLABLE_WITH` accepts the same syntax and performs the same
selection, then returns `TRUE` only when the selected declaration is a coroutine.
A synchronous function that accepts a `CONTROLLER` argument therefore produces
`FALSE`. The query preserves ambiguous-selection diagnostics.

## Function overload reflection

`FUNCTUM_OVERLOAD_COUNT(F)` returns a compile-time numeric literal containing the
number of function declarations in functum `F`. The count includes coroutine,
ordinary, disabled, private, and compiler-provided overloads. Function templates
require explicit template specialization before enumeration.

`FUNCTUM_GET_OVERLOAD(F, N)` returns the function declaration binding at the
zero-based, compile-time integer index `N`. The result can occur in a type-symbol
position or initialize a compile-time binding. An out-of-range index or an
inaccessible selected declaration produces a compilation error. Enumeration uses
the compiler's canonical overload order: user declarations precede builtin
entries. Indices identify declarations within the compiled source bundle; source
changes and target-specific builtin inventories can change these indices.

`FUNCTION_CAN_INSTANTIATE_WITH(F, @name Type, % [Type, ...])` tests specialization
of one selected function declaration. Its type arguments support the same named,
positional, and `COMPOSITE_UNPACK` forms as function specialization. The query
applies parameter deduction, adaptation, defaults, and declaration constraints.
It returns `FALSE` when specialization is unavailable. Invalid operands and access
violations produce compilation errors. The query does not validate the function
body. A coroutine's omitted controller uses specialization's deferred controller
semantics; ordinary invocation requires a controller argument.

`FUNCTION_IS_COROUTINE(F)` returns a constant `BOOL` indicating whether a selected
function declaration or instantiated function is a coroutine.

```quxlang
::operation FUNCTION(@value I32): I32 { RETURN value + 1; }
::operation COROUTINE(@value I32): I32 { RETURN value + 2; }

::inspect_overloads STATIC_TEST
{
  TEST_ASSERT(FUNCTUM_OVERLOAD_COUNT(operation) == 2);
  VAR first := FUNCTUM_GET_OVERLOAD(operation, 0);
  VAR second := FUNCTUM_GET_OVERLOAD(operation, 1);
  VAR ordinary := STATIC_CHOOSE(FUNCTION_IS_COROUTINE(first), second, first);
  TEST_ASSERT(FUNCTION_CAN_INSTANTIATE_WITH(ordinary, @value I32));
  VAR selected := ordinary#(@value I32);
  TEST_ASSERT(selected(@value 41) == 42);
}
```

A reflected declaration is specialized with `#(...)` before invocation. Member
specializations require an explicit `@THIS` type. These queries expose selection
tools to controller implementations; `CO_GO` determines its own selection policy.

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
The query requires a concrete type name or a resolved type argument. The query
does not currently resolve `THISTYPE`.

The trait can be used in `STATIC_IF`, assertions, and declaration conditions
such as `ENABLE_IF`. It requires neither object construction nor runtime RTTI
and is available on both native and layoutless targets. See the
[Overview examples](../overview/type-queries-and-deduction.md#polymorphism-queries).

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
[dynamic identity
example](../overview/type-queries-and-deduction.md#runtime-type-identification).

## Public struct fields

`TYPE_HAS_MEMBER(T, name)` tests instance-member name lookup on a structure or
generic type, including inherited names. The name is a compile-time string.
References are stripped from `T`; other type categories return `FALSE`.
The query reports name presence before overload selection or access checking.
An ordinary use of the member still checks ambiguity, accessibility, and
argument compatibility. This permits optional customization such as a promise's
`await_transform` without requiring a common base class.

[Public Field Reflection](public-field-reflection.md) inspects the fields of
named structs. `PUBLIC_FIELD_COUNT`, `PUBLIC_FIELD_NAME`, and
`PUBLIC_FIELD_CONTAINS` inspect the public field list; `PUBLIC_FIELD_TYPE`
reports a declared field type, and `PUBLIC_FIELD_GET` projects a field from
an object. Public field indices follow declaration order.

## Composite metadata

`COMPOSITE_CONTAINS`, `COMPOSITE_FIELD_COUNT`, `COMPOSITE_FIELD_NAME`, and
`COMPOSITE_FIELD_TYPE` inspect a composite type. `COMPOSITE_FIELD_GET` projects
a field from a value. Names and indices are selected at compile time; indices
enumerate positional members numerically, then named fields lexicographically. See [Composite Reflection](composites.md#static-reflection)
for the exact signatures and access qualifiers.

## Pack metadata

`PACK_CONTAINS`, `PACK_FIELD_COUNT`, `PACK_FIELD_NAME`, and `PACK_FIELD_TYPE`
inspect the schema of a pack binding. `PACK_TYPES` supplies its field types for
specialization expansion; `PACK_TYPES_EXCEPT` omits explicitly selected fields.
These operations preserve pack ownership. The [pack reference](packs.md)
specifies their selectors and failure conditions.

## Layout and integer properties

- `SIZEOF(T)` gives byte size where the type has a known size.
- `ALIGNOF(T)` gives alignment and is unavailable for layoutless types.
- `CELLSIZEOF(T)` gives object size rounded up to alignment, which is the stride
  between array elements. It is unavailable on layoutless targets.
- `BITS(T)` gives an integer type's bit width.
- `IS_SIGNED(T)` and `IS_INTEGRAL(T)` report integer properties.
- `TYPE_IS_LAYOUTLESS(T)` reports whether the current target omits a static byte
  layout for `T`.
- `BIT n` produces the compile-time numeric literal `2` raised to bit index
  `n`; cast it when a concrete runtime integer type is required.

Integer sizes and alignments are unspecified by the language, including for
standard-width integer types. `SIZEOF` and `ALIGNOF` report the selected target
object layout; `BITS` reports the value width independently of storage padding.
Object size and cell size are distinct: cell size includes any additional
spacing required between array elements to satisfy alignment. Equality between
`SIZEOF(T)` and the array element stride is not guaranteed.

`CELLSIZEOF(T)` accepts a type and produces a compile-time numeric literal.
The result is the smallest multiple of `ALIGNOF(T)` greater than or equal to
`SIZEOF(T)`. Zero-sized types have cell size zero. A result that exceeds the
target's object-size limit is rejected. For example,
`SIZEOF(ALIGNED_STORAGE(3, 2))` is 3 and
`CELLSIZEOF(ALIGNED_STORAGE(3, 2))` is 4. An array of `n` elements occupies
`n * CELLSIZEOF(T)` bytes, including padding between cells.

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

`AUTO_FLOAT(total, exponent)` matches a floating-point type and binds its total
bit count and exponent bit count as compile-time `U64` values. Repeated capture
names must have equal values throughout the matched parameter list. Integers
and untyped numeric literals do not match this pattern. Reference qualification
uses the surrounding reference pattern.

```quxlang
::float_identity FUNCTION(@ARG:value AUTO_FLOAT(total, exponent)):
  FLOAT(total, exponent)
{
  RETURN value;
}
```

For an `F64` argument, the captures are `total = 64` and `exponent = 11`.
`FLOAT(total, exponent)` reconstructs the captured type.

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

`AUTO(t)` deduces the non-reference value type. `TT(t)` retains the presented
reference shape. `DECAY(t)` converts an expiring reference to an owned value
type while retaining lvalue references as references.

See [Templates](templates-and-value-parameters.md),
[Variadic packs](variadic-packs.md), and
[Availability and targets](availability-and-targets.md).

## Local and member declaration queries

`DECLTYPE` resolves local variables, parameters, static bindings, and implicit
receiver fields such as `DECLTYPE(.value)`. It reports their declared type.
`TYPEOF` accepts complete expressions, including calls and `MOVE`, and reports
the expression's reference qualification without executing the operand:

```quxlang
VAR value I32 := 3;
TEST_ASSERT(SAME_TYPES(DECLTYPE(value), I32));
TEST_ASSERT(SAME_TYPES(TYPEOF(value), MUT& I32));
TEST_ASSERT(SAME_TYPES(TYPEOF(MOVE(value)), TEMP& I32));
```

## Lifecycle exception traits

The following expressions return a compile-time `BOOL` without constructing
objects or executing the selected operation:

| Expression | Operation selected for `T` |
| --- | --- |
| `IS_NOEXCEPT_MOVABLE(T)` | Construction from `TEMP& T` into new storage |
| `IS_NOEXCEPT_COPYABLE(T)` | Construction from `CONST& T` into new storage |
| `IS_NOEXCEPT_SWAPPABLE(T)` | `OPERATOR<->` on two `MUT& T` operands |

Each operand must name a type. Aliases resolve before selection. Ordinary
overload selection and enabled-overload conditions determine the operation.
An unavailable operation returns `FALSE`. These traits describe construction
and swap; assignment has a separate operation contract.

A user-defined operation returns `TRUE` when its declaration specifies
`NOEXCEPT` or an applicable `NOEXCEPT_IF(TRUE)`. An unannotated user function
returns `FALSE`, including a function with an empty body. Generated operations
are inspected through their generated routines, including calls to member and
base operations. Indirect calls and allocation paths without a proven
non-throwing contract return `FALSE`.

The traits can appear in `STATIC_IF`, `ENABLE_IF`, and `NOEXCEPT_IF` conditions.
Their evaluation performs compiler work and adds no runtime trait test.
