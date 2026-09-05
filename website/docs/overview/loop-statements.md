# Overview of `LOOP` Statements

Quxlang's clause-based `LOOP` statement handles ordinary test-and-step loops,
numeric sequences, and container iteration. Every form ends its header with
`DO` and then supplies the body.

## Test-and-step loops

Use `TEST` on its own to check a condition before each iteration:

```quxlang
VAR count I32 := 0;
LOOP TEST(count < 3) DO {
  count++;
}
ASSERT(count == 3);
```

Use `INIT`, `TEST`, and `STEP` for a conventional counted loop:

```quxlang
VAR total I32 := 0;

LOOP INIT { VAR index I32 := 0; }
    TEST(index < 5)
    STEP { index++; }
    DO
{
  total += index;
};

ASSERT(total == 10);
```

`INIT` runs once, `TEST` runs before each iteration, and `STEP` runs after the
body. `LOOP DO` is an unconditional loop; use `BREAK` to leave it.

`POSTTEST` checks after the body, so the body runs at least once when no
`TEST` clause is present:

```quxlang
VAR attempts I32 := 0;
LOOP POSTTEST(should_retry()) DO
{
  attempts++;
  perform_attempt();
};
```

## Numeric sequences

`VALUE`, `FROM`, and either `TO` or `UNTIL` describe a numeric sequence:

```quxlang
VAR inclusive_total I32 := 0;
LOOP VALUE(value) FROM(0 AS I32) TO(6) BY(2 AS I32) DO
{
  inclusive_total += value;
};
ASSERT(inclusive_total == 12);

VAR exclusive_total I32 := 0;
LOOP VALUE(value) FROM(0 AS I32) UNTIL(4) DO
{
  exclusive_total += value;
};
ASSERT(exclusive_total == 6);
```

`TO` includes the bound; `UNTIL` excludes it. `BY` defaults to one. Give the
`FROM` expression a concrete type, as in `0 AS I32`, because it determines the
type of the sequence variable, bound, and step.

Add `FILTER` to skip values without changing the sequence step:

```quxlang
LOOP VALUE(value) FROM(0 AS I32) UNTIL(10)
    FILTER((value % 2) == 0)
    DO
{
  use_even_value(value);
};
```

## Arrays and containers

`ITEM(name) IN(range)` iterates direct elements. When the iterator returns a
reference, the binding can modify the element:

```quxlang
VAR values [4]I32 :[1, 2, 3, 4];
VAR total I32 := 0;

LOOP ITEM(item) IN(values) DO
{
  total += item;
  item++;
};

ASSERT(total == 10);
ASSERT(values[0] == 2);
```

Container types participate through `.BEGIN()` and `.END()`. They may also
provide `.VALUES()`, `.INDEXES()`, and `.IV_PAIRS()` projections for
`VALUE`, `INDEX`, or combined `INDEX VALUE` loops.

Use `ITER(name)` when the body or a custom `STEP` needs the iterator itself:

```quxlang
LOOP ITER(iterator) ITEM(item) IN(container)
    FILTER(item != 0)
    STEP { iterator++; }
    DO
{
  consume(item);
};
```

The default iterator step is `iterator++`. `BY(amount)` uses
`iterator += amount` instead.

## Loop control

`CONTINUE` advances through the loop's normal post-test and step path. `BREAK`
exits. Labels let a nested loop target an outer loop:

```quxlang
LOOP :rows VALUE(row) FROM(0 AS I32) UNTIL(height) DO
{
  LOOP VALUE(column) FROM(0 AS I32) UNTIL(width) DO
  {
    IF (skip_remaining_columns(row, column))
    {
      CONTINUE :rows;
    }
  };
};
```

## Reference

See the [`LOOP` Statements Reference](../reference/loop-statements.md) for clause-form
selection, exact phase ordering, iterator protocols, binding scope, boundary
comparisons, and every invalid clause combination.
