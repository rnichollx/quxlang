# Overview of Operator Precedence

Precedence decides how an expression groups when parentheses do not say so.
Quxlang's ordering differs from C-family languages in several places, so use
parentheses whenever a mixed expression is not immediately clear.

## Recognize tight postfix operations

Calls, member access, indexing, and pointer operations bind tightly and can be
chained:

```quxlang
VAR field I32 := container.item_at(@index index).field;
VAR pointed I32 := pointers[index]->;
```

## Parenthesize mixed arithmetic

Multiplication binds more tightly than addition and subtraction, but addition
and subtraction bind more tightly than division and remainder:

```quxlang
VAR intended I32 := (left + right) / divisor;
```

That explicit grouping is especially valuable because readers familiar with C
may otherwise expect division to bind first.

## Separate operator families

Bitwise operators bind more tightly than arithmetic, comparisons bind after
arithmetic and conversions, and logical operators bind after comparisons:

```quxlang
VAR masked I32 := (value + offset) #&& mask;
VAR selected BOOL := (left < right) && enabled;
```

Assignment and swap bind most loosely. An operator overload uses the same
precedence as its built-in spelling; user-defined operators cannot introduce a
new precedence level.

## Complete technical rules

See the [Operator Precedence Reference](../../reference/operator-precedence.md)
for the complete ordered table, every postfix form, conversion and type-test
placement, and assignment grouping.
