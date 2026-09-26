# Overview of `LOOP WHILE` Loops

`LOOP WHILE` repeats a block while its condition is true. The condition is checked
before the first iteration and before every later one.

```quxlang
VAR index SZ := 0;
LOOP WHILE (index < count) DO
{
  process(index);
  index++;
}
```

If the condition starts false, the body does not run.

## Leaving or skipping an iteration

`BREAK` exits the loop. `CONTINUE` skips the rest of the body and tests the
condition again:

```quxlang
VAR index SZ := 0;
LOOP WHILE (index < count) DO
{
  index++;
  IF (should_skip(index))
  {
    CONTINUE;
  }
  IF (finished(index))
  {
    BREAK;
  }
  process(index);
}
```

The loop body must update loop state before `CONTINUE` if the condition depends
on that state.

## Loop labels

A loop label allows nested code to target an outer loop:

```quxlang
LOOP :records WHILE (has_record()) DO
{
  LOOP WHILE (has_field()) DO
  {
    IF (record_is_invalid())
    {
      CONTINUE :records;
    }
  }
}
```

A [`LOOP` statement](loop-statements.md) provides explicit step phases, numeric
sequences, filters, and container iteration.

## Reference

See the [`LOOP WHILE` Loops Reference](../reference/while-loops.md) for condition
conversion, exact `BREAK` and `CONTINUE` targets, labels, block scope, and
object-lifetime behavior.
