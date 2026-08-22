# Bitwise Operators

Bitwise operators apply to the fixed-width representation of integers and
`BYTE`. Their `#` prefix keeps them distinct from Boolean logical operators.

## Boolean-shaped bit operations

The binary operands have the same concrete type and the result has that type:

| Operator | Per-bit result |
| --- | --- |
| `a #&& b` | and |
| `a #\|\| b` | or |
| `a #^^ b` | exclusive or |
| `a #^! b` | equivalence, or not-exclusive-or |
| `a #&! b` | nand |
| `a #\|! b` | nor |
| `a #^> b` | `a` implies `b`, equivalent to `(#!!a) #\|\| b` |
| `a #^< b` | `b` implies `a`, equivalent to `a #\|\| (#!!b)` |

Every result bit is computed independently across the entire declared width:

```quxlang
VAR left BYTE := 6;   // 0000 0110
VAR right BYTE := 3;  // 0000 0011

ASSERT((left #&& right) == 2);
ASSERT((left #|| right) == 7);
ASSERT((left #^^ right) == 5);
ASSERT((left #&! right) == 253);
```

These operations do not short-circuit. Flagsets support the same
Boolean-shaped family with the result remaining the flagset type.

## Bitwise inverse

`value #!!` is suffix bitwise inverse and preserves the operand type:

```quxlang
VAR bits BYTE := 170; // 1010 1010
ASSERT((bits #!!) == 85);
```

All bits within the declared width are inverted. This is distinct from suffix
`!!`, which performs logical negation and returns `BOOL`.

## Shifts

```quxlang
VAR value I32 := 5;
ASSERT((value #++ 1) == 10);
ASSERT((value #-- 1) == 2);
```

- `#++` shifts bits toward higher-numbered positions and fills low positions
  with zero;
- `#--` shifts bits toward lower-numbered positions and fills high positions
  with zero.

Bits shifted beyond the declared width are discarded. Down-shift is a logical
bit shift even when the operand is a signed integer; it does not replicate the
sign bit. The shift amount has type `UINTPTR`. Keep an ordinary shift count
strictly below the operand width; use a rotate when wraparound is intended.

## Rotations

`#+%` rotates toward higher bit positions and `#-%` rotates toward lower bit
positions:

```quxlang
VAR bits BYTE := 129;       // 1000 0001
ASSERT((bits #+% 1) == 3);  // 0000 0011
ASSERT(((bits #+% 1) #-% 1) == bits);
```

Bits crossing one edge re-enter at the opposite edge. The amount has type
`UINTPTR` and is reduced modulo the operand width.

## Compound assignment

Every binary bitwise operator has a mutating form:

```quxlang
bits #&&= mask;
bits #||= additions;
bits #++= 3;
bits #-%= 1;
```

The complete set is `#&&=`, `#||=`, `#^^=`, `#^!=`, `#&!=`, `#|!=`,
`#^>=`, `#^<=`, `#++=`, `#--=`, `#+%=`, and `#-%=`. The left operand must be
writable. Its value is replaced with the corresponding non-mutating result;
the compound form does not introduce a different arithmetic contract.

## `BIT` literals

`BIT n` is a compile-time numeric literal whose only set bit is bit `n`:

```quxlang
ASSERT(BIT 0 == 1);
ASSERT(BIT 5 == 32);
ASSERT((BIT 63 AS U64) == 9223372036854775808);
```

The index must be a non-negative integer literal. The result initially has the
uncommitted numeric-literal category; cast or initialize it into a concrete
integer type whose width can represent the selected bit. A negative, fractional,
or missing index is rejected.

See [Assignment Operators](assignment-operators.md),
[Logical Operators](logical-operators.md), and
[Operator Precedence](operator-precedence.md).
