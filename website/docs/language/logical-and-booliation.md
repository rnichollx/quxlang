# Logical and Booliation Operators

## Boolean logic

```quxlang
ASSERT(TRUE && condition);
ASSERT(FALSE || condition);
ASSERT(TRUE ^^ FALSE);
ASSERT(TRUE ^! TRUE);
ASSERT((TRUE &! TRUE) == FALSE);
ASSERT((FALSE |! FALSE) == TRUE);
ASSERT((TRUE ^> FALSE) == FALSE);
ASSERT((TRUE ^< FALSE) == TRUE);
ASSERT((condition !!) == FALSE);
```

The binary logical family is:

| Operator | Meaning |
| --- | --- |
| `&&` | and |
| `||` | or |
| `^^` | exclusive or |
| `^!` | equivalence |
| `&!` | nand |
| `|!` | nor |
| `^>` | implication |
| `^<` | reverse implication |

`!!` is suffix logical negation. `&&`, `||`, and the implication operators can
avoid evaluating an operand when the result is already determined.

## Booliation

Types with an affirmative or empty state use explicit suffix tests rather than
implicit conversion to `BOOL`:

```quxlang
IF (pointer??)
{
  pointer-> := 1;
}

ASSERT((pointer?!) == FALSE);
```

- `value??` tests the affirmative state.
- `value?!` tests the corresponding zero, null, empty, valueless, or otherwise
  negative state.

Pointers, interfaces, enums, flagsets, fusion values, and supported numeric
types define the meaning appropriate to their category.

