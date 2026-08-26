# Overview of Logical Operators

Logical operators combine conditions and produce `BOOL` values. Quxlang uses
suffix operators for negation and for explicit presence tests.

## Combining conditions

```quxlang
::may_enter FUNCTION(@has_key BOOL, @door_open BOOL): BOOL
{
  RETURN has_key && door_open;
}

ASSERT(may_enter(@has_key TRUE, @door_open TRUE));
ASSERT((may_enter(@has_key TRUE, @door_open FALSE) !!));
```

The common operators are `&&` for and, `||` for or, `^^` for exclusive or,
and suffix `!!` for negation. Quxlang additionally provides nand (`&!`), nor
(`|!`), equivalence (`^!`), implication (`^>`), and reverse implication
(`^<`).

## Short-circuit evaluation

`&&` stops when its left operand is false, while `||` stops when its left
operand is true. This is useful when the right operand is only valid after the
left check succeeds:

```quxlang
IF (item?? && item-> > 0)
{
  item-> := item-> - 1;
}
```

Nand, nor, and the implication operators also skip the right operand whenever
the left operand already determines the result. Exclusive-or and equivalence
evaluate both operands.

## Testing whether a value is present

`value??` asks whether a supported value is present or nonzero. `value?!` asks
the opposite.

```quxlang
VAR item ->I32;
ASSERT(item?!);

VAR storage I32 := 7;
item := storage<-;
ASSERT(item??);
ASSERT(item-> == 7);
```

Pointers use null versus non-null, integers use zero versus nonzero, interfaces
use empty versus populated, and unions or variants use valueless versus holding
an alternative.

## Reference

See the [Logical Operators Reference](../reference/logical-operators.md) for
the full truth-table meanings, evaluation rules, and supported presence tests.
