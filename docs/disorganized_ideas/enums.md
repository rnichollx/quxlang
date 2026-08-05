# Enums

Enum binary represetation need not match the wire format. Default can have binary value 0,

e.g. `[a, b, c]` assigns 0, 1, 2.

DEFAULT is always 0, so to keep ordering, other values treated as signed, e.g.

```
::foo ENUM [a, b DEFAULT, c];
// a = -1, b = 0, c = 1
```

This allows translating enum values to a default offset:

```
offset = f + 1
offset( a = 0, b = 1, c = 2 )
```

This allows enums to be used in jump-tables.

