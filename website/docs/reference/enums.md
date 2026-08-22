# Enums

An enum is a nominal type with unique named integer values. Quxlang can assign
values automatically, reserve numeric ranges, designate default and null
values, and fix or infer the representation width.

## Declaration forms

```quxlang
::color ENUM BITS(8) [red, green, blue];
```

`IPC_ENUM` uses the same source grammar and marks the enum for the IPC-oriented
contract:

```quxlang
::message_kind IPC_ENUM BITS(16) [request = 1, response = 2];
```

The optional `BITS(expression)` is evaluated at compile time. It must be greater
than zero and large enough for every value and reserved range. Without `BITS`,
the smallest unsigned width that represents the highest used or reserved value
is inferred, with a minimum of one bit.

## Explicit and implicit values

An entry can provide an explicit nonnegative constant:

```quxlang
::state ENUM [idle = 0, running = 4, stopped];
```

Names and numeric values must both be unique. Explicit values must fit the
representation and cannot overlap a reserved range.

Entries without `=` are assigned in declaration order. Assignment begins at
zero and chooses the next unused, non-reserved value. An explicit value does not
reset this search; implicit allocation continues by finding the lowest
available candidate at or after the current candidate.

## Reserved ranges

`RESERVED FROM(first) TO(last)` excludes an inclusive numeric range:

```quxlang
::wire_state ENUM BITS(8)
  [ready = 0, RESERVED FROM(1) TO(15), running, stopped];
```

Both bounds are nonnegative compile-time integers, and `FROM` must not exceed
`TO`. Reserved ranges contribute to inferred width. They do not declare enum
values, and ordinary entries may not use a reserved number.

## `DEFAULT` and `NULL`

At most one value can be the default:

```quxlang
::mode ENUM [automatic DEFAULT, manual];
```

Default construction selects that value. `DEFAULT` may appear before or after
an entry's `= expression` clause.

`= NULL` declares the enum's null value:

```quxlang
::handle_state ENUM [none = NULL, active = 1];
```

The null value has numeric representation zero and also acts as the default.
Only one null value is permitted, and no other entry may use the same numeric
value. Declaring a separate incompatible `DEFAULT` is invalid.

If an enum has neither a default nor a null value, no generated default state is
available unless the type supplies another applicable construction path.

## Known and unknown values

By default, valid semantic values are the declared named values. The optional
`ALLOW_UNKNOWN` keyword follows the closing list:

```quxlang
::protocol_code ENUM BITS(16) [ok = 0, error = 1] ALLOW_UNKNOWN;
```

It permits the representation to carry values not assigned a canonical name,
which is useful for forward-compatible protocols. Reserved ranges remain part
of the protocol contract and are not canonical named values.

## Names, comparison, and conversion

Select a value as `enum_type::name`:

```quxlang
VAR current color := color::red;
ASSERT(current == color::red);
```

Values of the same enum support equality and ordering according to their
integer representations. An enum remains nominal: a matching integer width or
another enum does not make a value implicitly interchangeable. Explicit
conversion is required when raw representation access is part of the program's
contract.

An enum with a null value supports the ordinary valid/null observation
operators associated with that defaultable representation.

## Associated declarations and serialization

An enum can use a declaration body instead of the trailing semicolon:

```quxlang
::color ENUM BITS(8) [red, green, blue]
{
  ::is_primary FUNCTION(@ARG:value color): BOOL
  {
    RETURN value == red || value == green || value == blue;
  }
}
```

The body contains declarations owned by the enum type. Generated serialization
uses the enum's fixed or inferred integer width. `ALLOW_UNKNOWN`, reserved
ranges, and IPC intent remain semantic properties of the type rather than
changes to the list syntax.
