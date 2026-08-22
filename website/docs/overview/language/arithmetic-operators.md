# Overview of Arithmetic Operators

Quxlang uses `+`, `-`, `*`, `/`, and `%` for arithmetic. Concrete integer
types and `BYTE` support all five; floating-point types do not provide `%`.

```quxlang
VAR left I32 := 17;
VAR right I32 := 5;

ASSERT(left + right == 22);
ASSERT(left - right == 12);
ASSERT(left * right == 85);
ASSERT(left / right == 3);
ASSERT(left % right == 2);
```

## Explicit numeric types

Built-in runtime arithmetic expects compatible concrete operand types. Convert
different widths or signedness explicitly:

```quxlang
VAR narrow I16 := 10;
VAR wide I32 := 20;
VAR total I32 := (narrow AS I32) + wide;
```

Untyped literals remain compile-time literals until a target type is selected:

```quxlang
VAR large U128 := (100000000000000000000 + 7) AS U128;
```

## Pointer arithmetic

Array pointers advance in elements and can be subtracted to obtain a distance:

```quxlang
VAR values [4]I32 :[10, 20, 30, 40];
VAR begin MUT=>>I32 := values[& 0];
VAR third MUT=>>I32 := begin + (2 AS SZ);
ASSERT(third-> == 30);
```

Instance pointers do not support element arithmetic. `ADDRESS` arithmetic is
byte-oriented.

Types can define their own arithmetic with `.OPERATOR+`, `.OPERATOR-`, and the
other operator members.

## Reference

See the [Arithmetic Operators Reference](../../reference/arithmetic-operators.md)
for primitive signatures, literal behavior, pointer and address arithmetic,
reflected operators, and Quxlang's non-C precedence rules.

