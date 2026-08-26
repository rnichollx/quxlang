# Overview of Bitwise Operators

Bitwise operators have a `#` prefix so they remain visibly distinct from
logical operators:

```quxlang
VAR bits BYTE := 6;

bits := bits #&& 3;  // and
bits := bits #|| 3;  // or
bits := bits #^^ 3;  // xor
bits := bits #!!;    // inverse
bits := bits #++ 2;  // shift up
bits := bits #-- 1;  // shift down
bits := bits #+% 3;  // rotate up
bits := bits #-% 3;  // rotate down
```

## Boolean-shaped bit operations

The complete binary family is `#&&`, `#||`, `#^^`, `#^!`, `#&!`, `#|!`,
`#^>`, and `#^<`. They apply the corresponding Boolean truth table to every
bit.

Each has a compound-assignment form ending in `=`, such as `#&&=` and `#||=`.

## Shifts and rotations

`#++` and `#--` shift toward higher or lower bit positions. `#+%` and `#-%`
rotate rather than discard the bits that cross an edge. Their compound forms
are `#++=`, `#--=`, `#+%=`, and `#-%=`.

## Bit literals

`BIT n` constructs a compile-time numeric literal with one bit set:

```quxlang
ASSERT(BIT 0 == 1);
ASSERT(BIT 5 == 32);
ASSERT((BIT 63 AS U64) == 9223372036854775808);
```

The index is a non-negative integer literal. Cast the result to the required
concrete integer type when it enters runtime storage.

## Reference

See the [Bitwise Operators Reference](../reference/bitwise-operators.md) for the complete
language rules, constraints, and technical edge cases.
