# Overview of Increment and Decrement

Use suffix `++` and `--` to move a mutable value by one step:

```quxlang
VAR count I32 := 0;
count++;
count--;
```

The suffix expression returns the previous value while updating the operand:

```quxlang
VAR before I32 := count++;
```

Pointers use the same syntax to advance or retreat by one pointee:

```quxlang
VAR values [3]I32 :[2, 4, 6];
VAR cursor MUT=>>I32 := values.BEGIN();
cursor++;
ASSERT(cursor-> == 4);
```

Iterator-based `FOR` loops use suffix increment for their default step, so
custom iterator types can provide `.OPERATOR++`.

## Reference

`#++` and `#--` are bit shifts, not increment and decrement. For supported
operand types, pointer bounds, and user-defined dispatch, see the
[Increment and Decrement Reference](../../reference/increment-and-decrement.md).
