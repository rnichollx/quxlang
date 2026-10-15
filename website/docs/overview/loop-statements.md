# Overview of `LOOP` Statements

Quxlang's clause-based `LOOP` statement handles ordinary test-and-step loops,
numeric sequences, and container iteration. Every form ends its header with
`DO` and then supplies the body.

## Test-and-step loops

A `WHILE` clause checks a condition before each iteration:

```quxlang
VAR count I32 := 0;
LOOP WHILE(count < 3) DO {
  count++;
}
ASSERT(count == 3);
```

The combination of `INIT`, `WHILE`, and `STEP` expresses a conventional counted
loop:

```quxlang
VAR total I32 := 0;

LOOP INIT { VAR index I32 := 0; }
    WHILE(index < 5)
    STEP { index++; }
    DO
{
  total += index;
};

ASSERT(total == 10);
```

`INIT` runs once, `WHILE` runs before each iteration, and `STEP` runs after the
body. `LOOP DO` is an unconditional loop; use `BREAK` to leave it.

`POSTTEST` checks after the body, so the body runs at least once when no
`WHILE` clause is present:

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
LOOP VALUE(value I32) FROM(0) TO(6) BY(2) DO
{
  inclusive_total += value;
};
ASSERT(inclusive_total == 12);

VAR exclusive_total I32 := 0;
LOOP VALUE(value I32) FROM(0) UNTIL(4) DO
{
  exclusive_total += value;
};
ASSERT(exclusive_total == 6);
```

`TO` includes the bound; `UNTIL` excludes it. `BY` defaults to one. The type in
`VALUE(value I32)` determines the type of the sequence variable, bound, and
step. When the binding omits its type, the `FROM` expression must have a
concrete type, as in `VALUE(value) FROM(0 AS I32)`.

An index of type `SZ` can start from an untyped literal:

```quxlang
VAR count SZ := 4;
LOOP VALUE(index SZ) FROM(0) UNTIL(count) DO
{
  process_index(index);
};
```

A `FILTER` clause skips values without changing the sequence step:

```quxlang
LOOP VALUE(value I32) FROM(0) UNTIL(10)
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

LOOP ITEM(item MUT& I32) IN(values) DO
{
  total += item;
  item++;
};

ASSERT(total == 10);
ASSERT(values[0] == 2);
```

`VALUE`, `INDEX`, `ITEM`, and `ITER` accept a type after the binding name.
For container elements, a reference type such as `MUT& I32` retains access to
the original element. A value type such as `I32` constructs a separate object
from that element. An omitted type preserves the iterator's result.

Container types participate through `.BEGIN()` and `.END()`. They may also
provide `.VALUES()`, `.INDEXES()`, and `.IV_PAIRS()` projections for
`VALUE`, `INDEX`, or combined `INDEX VALUE` loops.

An `ITER(name)` clause exposes the iterator to the loop body and to a custom
`STEP` clause:

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
LOOP :rows VALUE(row I32) FROM(0) UNTIL(height) DO
{
  LOOP VALUE(column I32) FROM(0) UNTIL(width) DO
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
