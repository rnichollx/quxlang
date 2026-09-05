# Overview of `WHILE` Loops

`WHILE` repeats a block while its condition is true. The condition is checked
before the first iteration and before every later one.

```quxlang
VAR index SZ := 0;
WHILE (index < count)
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
WHILE (index < count)
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

Remember to update loop state before `CONTINUE` when the condition depends on
that state.

## Loop labels

Label an outer loop when nested code must target it:

```quxlang
WHILE :records (has_record())
{
  WHILE (has_field())
  {
    IF (record_is_invalid())
    {
      CONTINUE :records;
    }
  }
}
```

Use a [`LOOP` loop](loop-statements.md) for explicit step phases, numeric sequences,
filters, or container iteration.

## Reference

See the [`WHILE` Loops Reference](../reference/while-loops.md) for condition
conversion, exact `BREAK` and `CONTINUE` targets, labels, block scope, and
object-lifetime behavior.

