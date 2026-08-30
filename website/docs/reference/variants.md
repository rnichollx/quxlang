# Variants

A Quxlang variant stores one active alternative selected by payload type. Unlike
a [union](unions.md), a variant has no user-defined option names; its canonical
alternative types are the selectors used for construction, observation, and
`MATCH`.

## Declaration forms

`INLINE_VARIANT` stores its payload inside the variant object:

```quxlang
::number_or_void INLINE_VARIANT [I32 DEFAULT, VOID];
```

`VARIANT` uses boxed payload storage:

```quxlang
::recursive_value VARIANT [I32 DEFAULT, recursive_value];
```

An inline declaration ends with a semicolon after the closing `]` unless it has
an associated-declaration body. A boxed declaration follows the same rule.

The list must contain at least one type. After contextual type resolution, each
alternative type must be unique. At most one entry may be followed by
`DEFAULT`.

## Associated declarations

A variant may add a body after its alternative list:

```quxlang
::value INLINE_VARIANT [I32 DEFAULT, VOID]
{
  ::has_number FUNCTION(@ARG:item CONST& value): BOOL
  {
    RETURN item ISA I32;
  }
}
```

When the body is present, no semicolon follows it. Associated declarations are
owned by the variant type and follow the same member and type-owned declaration
rules as structures and unions.

## Construction

The generated default constructor selects the `DEFAULT` alternative and
default-constructs that payload:

```quxlang
VAR number number_or_void;
ASSERT(number ISA I32);
ASSERT((UNWRAP number INTO I32) == 0);
```

If no alternative is marked `DEFAULT`, ordinary default construction is
available only with `VALUELESS_DEFAULT` or through a suitable user-defined
constructor.

Passing a value to the `OTHER` constructor path selects the corresponding
alternative through constructor overload resolution:

```quxlang
VAR seven number_or_void := 7 AS I32;
VAR nothing number_or_void := NULL;
```

`NULL` has the `VOID` value role and selects the `VOID` alternative. Because
alternative types must be unique, a successful exact alternative selection is
unambiguous at the variant level. Ordinary conversion and overload-ranking
rules still determine which constructor is applicable.

User-defined constructors in the associated body participate in the same
selection and may outrank a generated constructor.

## `ISA`

`value ISA Type` tests whether `Type` is the active alternative:

```quxlang
ASSERT(seven ISA I32);
ASSERT((seven ISA VOID) == FALSE);
```

The subject must be a variant, and the named type must be one of its canonical
alternatives. A valueless variant returns `FALSE` for every `ISA` test.

`value??` reports that some alternative is active. `value?!` reports that the
variant is valueless.

## `UNWRAP`

`UNWRAP value INTO Type` returns a reference to the active payload:

```quxlang
VAR payload I32 := UNWRAP seven INTO I32;
```

The target type must be a non-`VOID` alternative of the variant. `UNWRAP` causes
a runtime failure if the variant is valueless or if another alternative is
active. It is therefore appropriate when the active type is already guaranteed
by the surrounding logic. Use [`MATCH`](match.md) when control flow must safely
distinguish alternatives.

The returned payload access preserves the subject's usable mutability. A
mutable variant can therefore expose a mutable payload; a constant subject does
not grant mutable access.

`VOID` cannot be unwrapped because it has no payload object. Test it with
`ISA VOID` or handle it with a `TYPE VOID` match arm.

## Valueless policies

Modifiers follow `VARIANT` or `INLINE_VARIANT` and precede the list:

```quxlang
::optional_number INLINE_VARIANT VALUELESS_DEFAULT [I32, VOID];
```

- `VALUELESS_DEFAULT` makes default construction produce no active
  alternative. It cannot be combined with a `DEFAULT` entry.
- `NEVER_VALUELESS` requires generated lifecycle operations to preserve an
  active alternative. It cannot be combined with `VALUELESS_DEFAULT`.

Without `NEVER_VALUELESS`, move construction may leave the source variant
valueless. With `NEVER_VALUELESS`, generated move behavior resets the source to
the default alternative, whose required construction operations must be
available.

## Generated lifecycle operations

Generated copy, move, assignment, swap, and destruction dispatch to the active
payload type. Their availability depends on the operations supported by the
alternatives.

The modifiers `NO_DEFAULT_COPY`, `NO_DEFAULT_MOVE`, `NO_DEFAULT_ASSIGN`, and
`NO_DEFAULT_SWAP` suppress the named generated operation. A user-defined
operation may replace a suppressed one; otherwise, uses that require it are
ill-formed.

Copying preserves the source alternative. Moving applies the declared
valueless policy to the source. Assignment replaces the destination's active
alternative. Swap exchanges alternatives and payloads, including valueless
state, and self-swap preserves the object. Destruction destroys the active
payload when one exists.

## Inline and boxed representation

`INLINE_VARIANT` requires enough inline layout for every alternative.
`VARIANT` owns boxed payload storage and permits recursive alternatives that
cannot be embedded directly. Representation does not change `ISA`, `UNWRAP`,
construction, or `MATCH` syntax, but it is part of the declared type's layout
and ABI.

See [Unions](unions.md) for name-selected alternatives and [`MATCH`](match.md)
for guarded and exhaustive handling. See [`VISIT`](visit.md) to specialize one
common region over every non-`VOID` variant alternative.
