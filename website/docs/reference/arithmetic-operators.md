# Arithmetic Operators

Quxlang's arithmetic operators are `+`, `-`, `*`, `/`, and `%`. Their meaning
comes from the operand type's operator contract; the primitive numeric types
provide the built-in forms listed here.

## Primitive arithmetic

Integers and `BYTE` support all five binary operators with operands and result
of the same concrete type:

```quxlang
VAR left I32 := 17;
VAR right I32 := 5;

ASSERT(left + right == 22);
ASSERT(left - right == 12);
ASSERT(left * right == 85);
ASSERT(left / right == 3);
ASSERT(left % right == 2);
```

Floating-point types support `+`, `-`, `*`, and `/`; `%` has no built-in
floating-point overload.

```quxlang
VAR numerator F64 := 7.5;
VAR denominator F64 := 2.5;
ASSERT(numerator / denominator == 3.0);
```

Primitive runtime arithmetic does not apply the usual C integer promotions.
The built-in overload takes two values of the same concrete integer,
`BYTE`, or floating-point type and returns that type. Convert an operand
explicitly when widths or signedness differ.

```quxlang
VAR narrow I16 := 10;
VAR wide I32 := 20;
VAR total I32 := (narrow AS I32) + wide;
```

See [Conversions](conversions.md) for `CHECKED`, `PARTIAL`, and other
conversion modes.

## Numeric literals

Two untyped numeric literals are evaluated as arbitrary-precision compile-time
literals. The result remains a literal until a construction or conversion
selects a concrete type:

```quxlang
VAR value U128 := (100000000000000000000 + 7) AS U128;
```

A literal used with a concrete operand is adapted through ordinary overload
resolution. It must fit the target type when that conversion requires an exact
value.

## Unary signs

Prefix `+` preserves a numeric value and prefix `-` negates it where the type
provides the corresponding unary operation. Parenthesize a negative value when
it participates in a conversion or a lower-precedence binary expression:

```quxlang
VAR negative I32 := 0 - 7;
VAR widened I64 := (0 - 7) AS I64;
```

## Pointer and address arithmetic

Array pointers (`=>>T`) support element-based arithmetic:

| Expression | Result |
| --- | --- |
| `pointer + offset` | Array pointer advanced by `offset` elements |
| `pointer - offset` | Array pointer retreated by `offset` elements |
| `left - right` | Signed pointer-sized element distance |

The offset may be the target's signed or unsigned pointer-sized integer. The
pointers must refer into the same valid array object. Instance
pointers (`->T`) do not provide pointer arithmetic.

`ADDRESS` provides byte-oriented arithmetic instead. `address + size` and
`address - size` produce another `ADDRESS`; subtracting two `ADDRESS` values
produces an unsigned pointer-sized byte distance.

See [Pointers](pointers.md) for validity rules that arithmetic does not
override.

## User-defined arithmetic

For `left operator right`, Quxlang first looks for an applicable
`left_type::.OPERATOR<operator>` and then for the reflected
`right_type::.OPERATOR<operator>RHS`. Normal overload resolution and argument
adaptation select the callable declaration.

```quxlang
::distance STRUCT
{
  .meters VAR I32;

  .OPERATOR+ FUNCTION(@OTHER CONST& distance): distance
  {
    RETURN distance(@meters .meters + OTHER.meters);
  }
}
```

An overload changes the operation for its types but not the parser's precedence
or associativity. See [User-Defined Operators](user-defined-operators.md).

## Precedence

Quxlang's current precedence differs from C-family languages: `*` binds more
tightly than `+` and `-`, while `+` and `-` bind more tightly than `/` and `%`.
Use [Operator Precedence](operator-precedence.md) when mixing arithmetic forms.
