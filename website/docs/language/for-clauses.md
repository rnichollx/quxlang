# `FOR` Clauses

Quxlang's `FOR` statement is assembled from named clauses and ends its header
with `LOOP`.

## Explicit phases

```quxlang
FOR INIT { VAR index I32 := 0; }
    TEST(index < 5)
    STEP { index++; }
    LOOP
{
  total += index;
};
```

`INIT` and `STEP` contain statement blocks. `TEST` is checked before an
iteration; `POSTTEST` is the corresponding post-body condition. `EVAL` supplies
an additional evaluation block.

With no clauses, `FOR LOOP` is an unconditional loop whose exit must come from
its body:

```quxlang
FOR LOOP
{
  IF (finished)
  {
    BREAK;
  }
};
```

## Numeric sequences

```quxlang
FOR VALUE(index) FROM(0 AS I32) UNTIL(5) BY(1 AS I32) LOOP
{
  total += index;
};
```

`TO` includes its bound, while `UNTIL` excludes it. `BY` specifies the step.

## Containers and iterators

```quxlang
FOR ITEM(item) IN(values) FILTER(item != 0) LOOP
{
  total += item;
};
```

Lower-level iterator forms can name `ITER`, `START`, `END`, and `LIMIT`, while
container protocols can expose `ITEM`, `INDEX`, or both.

### The `IN` projection protocol

An `IN(range)` clause chooses a projection from the bound names, then calls
`BEGIN()` and `END()` on the projected range:

| Bound clauses | Projection before iteration |
| --- | --- |
| `ITEM(item)` | Use `range` directly |
| `VALUE(value)` | Call `range.VALUES()` |
| `INDEX(index)` | Call `range.INDEXES()` |
| `INDEX(index) VALUE(value)` | Call `range.IV_PAIRS()` |

The iterator returned by `BEGIN()` supports comparison with the end iterator,
dereference through `->`, and advancement through `++` or `+=` when `BY` is
used. For an index/value projection, the dereferenced item supplies `INDEX()`
and `VALUE()` members.

Built-in arrays provide the value projection:

```quxlang
VAR values [3]I32 :[10, 20, 30];
VAR total I32 := 0;

FOR VALUE(value) IN(values) LOOP
{
  total += value;
};

ASSERT(total == 60);
```

Custom containers can return distinct view objects from `.VALUES`, `.INDEXES`,
and `.IV_PAIRS`; the names do not force an index representation. This keeps the
loop syntax independent of a container's iterator and projection types.

The complete implemented clause vocabulary is `INIT`, `EVAL`, `TEST`,
`POSTTEST`, `STEP`, `ITER`, `VALUE`, `INDEX`, `ITEM`, `IN`, `START`, `END`,
`LIMIT`, `FILTER`, `BY`, `FROM`, `TO`, and `UNTIL`. Each clause may occur at
most once. Clauses may be reordered when their combination is meaningful.

See [Labels and `GOTO`](labels-and-goto.md) for labeled loop exits.
