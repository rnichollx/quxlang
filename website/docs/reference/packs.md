# Packs

A pack is a generation-time binding containing independent field values. Each
field has a name, a type, and a value category. A pack has no aggregate runtime
storage. A field can own an object, bind a reference, or contain another pack.

A [composite](composites.md) is an ordinary generated struct with aggregate
storage. Pack declarations and operations specify argument assembly and
ownership transfer.

## Declarations and literals

```quxlang
PACK named ...{ .first = I32(@OTHER 19), .second = I32(@OTHER 23) };
PACK positional ...[19, 23];
PACK empty ...{};
PACK nested ...{ .arguments = ...{ .value = 42 } };
```

Named literals accept `.name = expression`, `.name: expression`, and numeric
members such as `[0] = expression`. Fields are evaluated once in source order.
Numeric members are ordered numerically before lexicographically ordered named
members. A literal's numeric members form a contiguous sequence beginning at
zero. Field names are unique.

An owned initializer supplies its genvalue directly. A reference initializer
retains its reference type. Pack construction does not construct an intermediate
composite or invoke field copy or move constructors. Addressable fields can
have ordinary storage independently of other fields. A nested pack initializer
transfers the nested pack binding into the new field.

Pack operands are accepted by pack declarations, pack operations, and argument
expansions. An ordinary `VAR` initializer, value parameter, return value, or
member-access expression requires an ordinary value. `PACK name := source;`
and `pack.field` are invalid.

## Parameter packs

A positional parameter pack uses `%...name T`. The parameter type `TT`
accepts the complete supplied type, including its reference category:

```quxlang
::relay FUNCTION(%...values TT, @KWARGS ...): I32
{
  RETURN target(...FORWARD PACK(values), ...FORWARD PACK(KWARGS));
}
```

`@KWARGS ...` captures unmatched named arguments. `@KWARGS:name ...` supplies
an explicit local name. Explicit parameters consume their matching arguments
before the named pack is formed. An empty pack is valid.

## Named pack parameters

A named parameter declared with `...` accepts one complete pack. Multiple
named pack parameters can appear in the same function declaration.

```quxlang
::apply FUNCTION(@funcs ..., @args ..., @KWARGS ...): I32
{
  VAR callback EMIT FIELD("callback") FROM (funcs);
  RETURN FORWARD PACK(args) TO callback;
}

apply(@funcs ...{ .callback = consume },
      @args ...{ .value = make_value() },
      @option TRUE);
```

Explicit named pack parameters bind before unmatched named arguments enter
`KWARGS`. Each parameter has a separate schema and ownership state. The call
passes independent fields through the parameter ABI without an intermediate
composite. Nested packs retain their field structure. Expanding a pack containing named
pack fields supplies those fields to the corresponding named pack parameters.

A bare named pack argument supplies borrowed fields and preserves the caller's
pack. `@args FORWARD PACK(arguments)` consumes the caller's pack and transfers
its owned fields. A fresh pack literal transfers its fields directly. An
ordinary value cannot bind to a named pack parameter.

## Calls and construction

```quxlang
PACK arguments ...{ .value = I32(@OTHER 42) };
consume(...arguments);
FORWARD PACK(arguments) TO consume;
```

`...arguments` expands a named pack without consuming its binding. Each owned
field is supplied through ordinary lvalue parameter adaptation. Reference
fields retain their declared reference categories. A by-value parameter may
therefore require a copy constructor.

`FORWARD PACK(arguments) TO callable` consumes the pack and forwards its field
genvalues. The pack operand is evaluated before the callable. Each owned field
can initialize a value parameter directly. Reference fields retain their
reference categories. The source pack cannot subsequently be used.

`...FORWARD PACK(arguments)` supplies consuming expansion within an argument
list. It is accepted by function calls, constructor calls, constructor
delegates, `NEW`, and `PLACE`. A fresh literal expansion such as
`consume(...{ .value = make_value() })` also consumes its independent fields.
Duplicate named arguments and reserved `RETURN` arguments are rejected.

## Fusion and fission

```quxlang
PACK first ...{ .a = 1, .b = 2 };
PACK selected FISSION FIELDS("b") FROM (first);
PACK combined FUSE(first, selected);
```

`FISSION` removes the selected fields from its named source and binds them to
the declared pack. The source becomes the remainder. Each selector is a
compile-time string naming an existing field. Repeated selectors are invalid.

`FUSE` consumes its source packs and binds their fields to one pack. Named
fields must be unique across the operands. Positional fields are concatenated
in operand order. Both operations transfer genvalues without invoking field
constructors.

## Extraction and capture

```quxlang
VAR value EMIT FIELD("value") FROM (arguments);
CAPTURE(value) NAMED("renamed") INTO(arguments);
PACK child EMIT FIELD("child") FROM (nested);
```

`EMIT` removes one field and binds its existing genvalue to the declared name.
A `VAR` binding retains the extracted field's type, including a reference type.
A nested pack requires a `PACK` binding. Extraction does not convert an owned
field to a prvalue or invoke a constructor. The extracted binding owns the field
until that binding is transferred or its scope ends.

`CAPTURE` removes a named local binding and attaches its genvalue to the named
destination pack. The destination field name is a compile-time string and must
be absent from the destination. Capturing a reference transfers the reference
binding; the referent retains its existing owner. An owned captured value follows
the destination pack's scope. A consumed local name can be declared again.

## Inspection

| Operation | Result |
| --- | --- |
| `PACK_FIELD_COUNT(pack)` | Number of fields. |
| `PACK_FIELD_NAME(pack, index)` | Field name at a canonical ordinal. |
| `PACK_FIELD_TYPE(pack, selector)` | Declared field type selected by name or ordinal. |
| `PACK_CONTAINS(pack, name)` | Whether the schema contains the named field. |
| `PACK_FIELD_GET(pack, selector)` | Ordinary borrowed access to a field selected by name or ordinal. |
| `PACK_SIZE(pack)` | Number of fields. |
| `PACK_ARG(pack, index)` | Ordinary borrowed access to a positional field. |

Selectors and indices are compile-time values. Field access leaves the pack
binding available. A nested pack is extracted with `PACK ... EMIT` before its
fields are accessed or forwarded.

## Type expansion and stored composites

`...PACK_TYPES(pack)` expands the declared field types in a specialization or
callability query. `...PACK_TYPES_EXCEPT(pack, "field", ...)` omits selected
fields from that type expansion without changing the pack. Excluded names must
exist and must be unique.

`COMPOSITE_TO_PACK(record)` creates a pack of field projections from an ordinary
composite. The projections follow the record expression's reference category.
A temporary-qualified record supplies temporary-qualified projections for its
owned fields. Existing reference fields preserve their declared types. The
record and its referents must outlive uses of the projections. This operation
supports invocation of stored argument records.

## Lifetime and control flow

Each owned field has an independent lifetime. Unconsumed fields are destroyed
when their owning scope ends. Consuming invocation transfers owned value
arguments to their parameters and retires the source field bindings after the
call. A field borrowed during a consuming call is kept alive through the call.

A transfer is rejected when an enclosing exception handler's entry state
requires the transferred source to be live. Pack ownership and field selection
must agree at control-flow joins. Pack storage and ownership operations are
resolved during generation and do not introduce runtime ownership checks.

Coroutine controller hooks receive their `ARGUMENTS` parameter as a pack.
Ordinary owned call operands are supplied through temporary references so the
calling expression retains their lifetimes across suspension. Explicit named
pack operands preserve their independent field ownership. Frame initialization
applies the coroutine parameter-capture rules when constructing frame storage.
Consuming a pack of
references transfers the reference bindings and does not transfer ownership
of the referenced objects.
