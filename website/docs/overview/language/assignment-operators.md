# Overview of Assignment Operators

Use `:=` to replace the value of an existing variable or member:

```quxlang
VAR score I32 := 10;
score := 25;
```

Equality uses `==`, so an assignment is visually distinct from a comparison:

```quxlang
IF (score == 25)
{
  score := 30;
}
```

## Declaration initialization

The same token appears in a declaration:

```quxlang
VAR copy I32 := score;
```

Here it initializes a new object through a constructor. Later uses such as
`copy := score;` assign an already-live object.

## Compound assignment

Compound forms combine an operation with assignment:

```quxlang
score += 5;
score -= 2;
score *= 3;
score /= 2;
score %= 7;
```

Integer and `BYTE` values also have compound bitwise shifts, rotates, and
logical bit operations.

## User-defined types

Structures receive generated memberwise assignment when their members support
it. A type can declare `.OPERATOR:=` when it needs a custom assignment contract.

Assignment changes a live object; it does not destroy and reconstruct it. For
operator dispatch, generated assignment, compound spellings, and lifetime
rules, see the [Assignment Operators Reference](../../reference/assignment-operators.md).
