# Overview of Move Semantics

Move semantics transfer an object's owned state instead of copying it. In
Quxlang, a move constructor receives the source as `TEMP&`:

```quxlang
.CONSTRUCTOR FUNCTION(@OTHER TEMP& buffer)
{
  .data <-> OTHER.data;
  .size <-> OTHER.size;
}
```

The source remains a live object after the constructor and will still be
destroyed. The move constructor must leave it in a safe moved-from state.

## Forwarding a reference category

Generic code uses `FORWARD(name)` to preserve whether a reference is mutable,
constant, or temporary:

```quxlang
::relay FUNCTION(@ARG:value AUTO& AUTO): DECLTYPE(value)
{
  RETURN FORWARD(value);
}
```

`FORWARD` does not move by itself. It passes the reference category onward so a
later call can choose a copy or move overload.

## Movement is not assignment

`destination := source;` assigns an existing object. Constructing a new object
from a forwarded temporary is what selects a move constructor:

```quxlang
VAR destination buffer := FORWARD(source);
```

For `TEMP&`, forwarding constraints, moved-from lifetime, and generated move
rules, see the [Move Semantics Reference](../../reference/move-semantics.md).
