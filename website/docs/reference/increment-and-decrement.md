# Increment and Decrement

The suffix operators `++` and `--` update an object by one step and produce the
operator result.

```quxlang
counter++;
counter--;
```

The operand must provide mutable access. Suffix `++` dispatches to
`OPERATOR++`; suffix `--` dispatches to `OPERATOR--`. Built-in numeric behavior
returns the prior value while updating the operand.

## Numeric values

For integers, floating-point values, and `BYTE`, increment adds one and
decrement subtracts one using the type's built-in arithmetic behavior. The
usual overflow and floating-point rules of that type apply.

```quxlang
VAR index U32 := 4;
VAR previous U32 := index++;
ASSERT(previous == 4);
ASSERT(index == 5);
```

## Pointers and iterators

For an instance pointer or array pointer, `++` advances by one pointee and `--`
retreats by one pointee. The operation preserves the pointer type and qualifier
and remains subject to the pointer's valid sequence and bounds:

```quxlang
VAR values [3]I32 :[2, 4, 6];
VAR cursor MUT=>>I32 := values.BEGIN();
cursor++;
ASSERT(cursor-> == 4);
```

Iterator-based `FOR` uses `iterator++` as its default advancement operation.
Custom iterator types therefore provide suffix increment and dereference
operators as part of their iteration contract.

## User-defined operators

A user type can declare `.OPERATOR++` or `.OPERATOR--`. The function receives
the operand as `THIS` and is responsible for performing the mutation and
returning the documented result type. The call participates in ordinary
overload selection.

These operators are not the bitwise shifts `#++` and `#--`. Shift expressions
do not mutate their left operand unless written as `#++=` or `#--=`; see
[Bitwise Operators](bitwise-operators.md).
