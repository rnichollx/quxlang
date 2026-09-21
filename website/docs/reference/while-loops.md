# `LOOP WHILE` Loops

`LOOP WHILE` repeatedly executes a block while a condition remains true.

```text
LOOP [':' label] WHILE '(' condition ')' DO block
```

The condition is tested before every iteration, including the first. If it is
false initially, the body does not execute.

```quxlang
VAR index SZ := 0;
LOOP WHILE (index < count) DO
{
  process(index);
  index++;
}
```

The condition must have type `BOOL` or be implicitly convertible to `BOOL`.
Side effects in the condition occur once per attempted iteration.

## `BREAK` and `CONTINUE`

`BREAK;` exits the nearest enclosing loop. `CONTINUE;` leaves the current body
execution and returns directly to the condition test.

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

Because `CONTINUE` does not supply an implicit step, any update needed for
progress must occur before it or inside the next condition evaluation.

## Labeled loops

A loop label appears immediately after `LOOP`. Labeled `BREAK` and
`CONTINUE` can target that loop from a nested construct:

```quxlang
LOOP :records WHILE (has_record()) DO
{
  LOOP :fields WHILE (has_field()) DO
  {
    IF (record_is_invalid())
    {
      CONTINUE :records;
    }
    IF (all_records_are_complete())
    {
      BREAK :records;
    }
  }
}
```

`CONTINUE :records` transfers to the condition of the labeled `LOOP WHILE`.
`BREAK :records` transfers to the statement after it. Labels must resolve to an
enclosing loop for `CONTINUE`; a labeled block can also be a target of
`BREAK`, as described under [Labels and `GOTO`](labels-and-goto.md).

## Body scope and lifetime

The body is a function block. Objects declared in it are created on each path
that reaches their declaration and leave scope before the next condition test,
on `CONTINUE`, or on `BREAK`. References retained outside the body must not
outlive the objects they name.

Additional [`LOOP` clauses](loop-statements.md) provide explicit initialization,
a post-test, a step block, numeric bounds, filtering, and iterator projection.
Use `STATIC_WHILE` for compile-time repetition.

