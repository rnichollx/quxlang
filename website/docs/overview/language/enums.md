# Overview of Enums

An enum is a nominal set of named values:

```quxlang
::color ENUM [red = 1, green DEFAULT, blue];

VAR selected color := color::red;
VAR defaulted color;
ASSERT(defaulted == color::green);
```

An entry may have an explicit value. Remaining values are assigned around
explicit and reserved ranges.

## Width, defaults, and reservations

```quxlang
::wire_state ENUM BITS(3)
  [idle DEFAULT = 0,
   ready = 1,
   RESERVED FROM(2) TO(3),
   done = 4]
  ALLOW_UNKNOWN;
```

- `BITS(N)` fixes the serialized width.
- One entry may be marked `DEFAULT` for default construction.
- `RESERVED FROM(a) TO(b)` prevents implicit allocation in an inclusive range.
- `ALLOW_UNKNOWN` permits representation values that have no named entry.

Without `ALLOW_UNKNOWN`, deserialization rejects unknown and reserved values.

## `IPC_ENUM`

`IPC_ENUM` has the same surface while guaranteeing that its in-memory
representation matches the declared integer values for external interfaces:

```quxlang
::seek_origin IPC_ENUM BITS(32)
  [begin DEFAULT = 0, current = 1, end = 2];
```

Enums support comparison, booliation where defined, and `.SERIALIZE` /
`.DESERIALIZE` using their configured representation.

## Complete technical rules

See the [Enums Reference](../../reference/enums.md) for the complete
language rules, constraints, and technical edge cases.
