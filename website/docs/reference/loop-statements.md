# `LOOP` Statements

Quxlang uses one clause-based `LOOP` statement for conventional loops, numeric
sequences, and iterator traversal. A header begins with `LOOP`, may carry a loop
label and clauses, and ends with `DO` followed by the body.

```text
LOOP [':' label] { clause } DO block [';']
```

The semicolon after the loop block is optional. Each clause may appear at most
once. Clauses may be written in any order, but only the combinations described
on this page are valid.

## Loop form selection

The clauses select one of three loop forms:

| Form | Selecting clauses |
| --- | --- |
| Conventional | No iterator or sequence clause |
| Numeric sequence | `VALUE`, `BY`, `FROM`, `TO`, or `UNTIL`, without an iterator-selecting clause |
| Iterator | `ITER`, `INDEX`, `ITEM`, `IN`, `START`, `END`, or `LIMIT` |

`VALUE` belongs to a numeric sequence when used with `FROM`. It is an iterator
projection when an iterator-selecting clause such as `IN` is also present.

## Conventional loops

A conventional loop can use these clauses:

| Clause | Meaning |
| --- | --- |
| `INIT { statements }` | Execute once when entering the loop. |
| `EVAL { statements }` | Execute once after `INIT`. |
| `TEST(condition)` | Require a true `BOOL` condition before each body execution. |
| `FILTER(condition)` | Skip the body when the `BOOL` condition is false. |
| `POSTTEST(condition)` | Continue only when the condition is true after the body. |
| `STEP { statements }` | Execute after each completed or skipped iteration. |

This is the direct equivalent of a conventional initialize-test-step loop:

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

The execution order is `INIT`, `EVAL`, then repeated
`TEST`, `FILTER`, body, `POSTTEST`, and `STEP`. Missing phases are omitted.
`FILTER(FALSE)` skips the body but still follows the normal continuation path,
including `POSTTEST` and `STEP` when present.

`LOOP DO` has no implicit condition and repeats until control leaves the loop:

```quxlang
LOOP DO
{
  IF (finished)
  {
    BREAK;
  }
  process_next();
};
```

`POSTTEST` makes the loop post-tested. Without `TEST`, the body executes before
the first post-test:

```quxlang
VAR count I32 := 0;
VAR index I32 := 0;

LOOP POSTTEST(index < 3) STEP { index++; } DO
{
  count++;
};

ASSERT(count == 4);
```

## Numeric sequence loops

A numeric sequence requires all of the following:

- `VALUE(name)` binds the current sequence value.
- `FROM(expression)` supplies the first value.
- Exactly one of `TO(expression)` or `UNTIL(expression)` supplies the bound.

`TO` is inclusive and tests the current value with `<=`. `UNTIL` is exclusive
and tests with `<`. `BY(expression)` supplies the increment; when omitted, the
increment is one.

```quxlang
VAR inclusive_total I32 := 0;
LOOP VALUE(value) FROM(0 AS I32) TO(10) BY(2 AS I32) DO
{
  inclusive_total += value;
};
ASSERT(inclusive_total == 30);

VAR exclusive_total I32 := 0;
LOOP VALUE(value) FROM(0 AS I32) UNTIL(5) DO
{
  exclusive_total += value;
};
ASSERT(exclusive_total == 10);
```

The `FROM` expression determines the sequence type. It must have a concrete
type, so an untyped numeric literal must be converted, for example with
`0 AS I32`. The bound and step are constructed as that type. The compiler does
not infer a descending comparison from a negative step: sequence continuation
still uses `<=` or `<`, and each step still applies `current + BY`.

`FILTER` is the only general phase clause accepted by a numeric sequence. A
sequence loop cannot also contain `INIT`, `EVAL`, `TEST`, `POSTTEST`, or
`STEP`.

```quxlang
VAR even_total I32 := 0;
LOOP VALUE(value) FROM(0 AS I32) TO(6)
    FILTER((value % 2) == 0)
    DO
{
  even_total += value;
};
ASSERT(even_total == 12);
```

The `VALUE` binding exists only inside the loop. `CONTINUE` advances by `BY`
before the next bound test.

## Iterator loops

An iterator loop obtains its initial iterator in one of two ways:

- `IN(range)` selects a range and obtains its iterator and boundary through
  `.BEGIN()` and `.END()`.
- `START(expression)` supplies an iterator directly. It may be paired with
  `END(expression)`, `LIMIT(expression)`, or neither.

`IN` cannot be combined with `START`, `END`, or `LIMIT`. Without `IN`, `START`
is required. `END` and `LIMIT` are mutually exclusive.

`END` terminates when `iterator != end` becomes false. `LIMIT` continues while
`iterator < limit`. A `START` loop with neither one is unbounded.

```quxlang
LOOP ITER(iterator) ITEM(item)
    START(container.BEGIN())
    END(container.END())
    DO
{
  consume(item);
};
```

The iterator is copied from the `START` or `.BEGIN()` result and has the
concrete non-reference type of that expression. `ITER(name)` exposes that
mutable loop iterator to the body. All iterator binding names must be distinct.

### Range projections

`IN(range)` chooses a projection from the requested bindings before it calls
`.BEGIN()` and `.END()`:

| Requested bindings | Range used for iteration |
| --- | --- |
| `ITEM(item)` or no item projection | `range` |
| `VALUE(value)` | `range.VALUES()` |
| `INDEX(index)` | `range.INDEXES()` |
| `INDEX(index) VALUE(value)` | `range.IV_PAIRS()` |

`ITEM` cannot be combined with `INDEX` or `VALUE`. `ITER` may accompany any
projection because it names the iterator rather than the dereferenced value.

Arrays support direct item iteration and value projection:

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
ASSERT(values[3] == 5);
```

The result of iterator `OPERATOR->` determines the binding:

- `ITEM`, `INDEX`, or `VALUE` alone receives the dereferenced result of the
  selected range.
- With both `INDEX` and `VALUE`, the dereferenced result must provide `.INDEX()`
  and `.VALUE()`; their results become the two bindings.

The names `VALUES`, `INDEXES`, and `IV_PAIRS` describe protocol members. They do
not require a particular container representation or require indices to be
integers.

### Iterator advancement

The default step applies postfix `++` to the iterator. `BY(expression)` instead
applies `iterator += expression`. A `STEP { statements }` clause replaces the
automatic step entirely and may use an `ITER` binding to update the iterator.
`BY` and `STEP` cannot appear together.

```quxlang
LOOP ITER(iterator) ITEM(item) IN(container)
    STEP { iterator++; }
    DO
{
  consume(item);
};

LOOP ITEM(item) IN(container) BY(2 AS SZ) DO
{
  consume_every_second_item(item);
};
```

For an `IN` loop with the default `++` step, the `.END()` result is an `END`
boundary and uses `!=`. When `BY` or `STEP` can advance past the exact end, the
same `.END()` result is treated as a `LIMIT` boundary and uses `<`.

### Iterator phase order

`INIT`, `EVAL`, the range or explicit boundary expressions, and `BY` are
evaluated once on loop entry. Each iteration then follows this order:

1. Check `END` with `!=` or `LIMIT` with `<`, when present.
2. Dereference the iterator and establish `ITEM`, `INDEX`, and `VALUE` bindings.
3. Evaluate `TEST`, when present; false exits the loop.
4. Evaluate `FILTER`, when present; false skips the body.
5. Execute the body.
6. Recheck the boundary before `POSTTEST` when a post-test is present.
7. Evaluate `POSTTEST`; false exits the loop.
8. Execute `STEP`, apply `+= BY`, or apply postfix `++`.

The boundary recheck before `POSTTEST` matters when the body or a `CONTINUE`
path changes an exposed iterator.

## `BREAK`, `CONTINUE`, labels, and scope

`BREAK` exits the nearest loop. `CONTINUE` follows that loop form's normal
continuation path: post-test and step for a conventional or iterator loop, and
the increment for a sequence loop.

A label follows `LOOP` and can be named by `BREAK` or `CONTINUE`:

```quxlang
LOOP :outer VALUE(row) FROM(0 AS I32) UNTIL(4) DO
{
  LOOP VALUE(column) FROM(0 AS I32) UNTIL(4) DO
  {
    IF (should_skip_remaining_columns(row, column))
    {
      CONTINUE :outer;
    }
  };
};
```

Names introduced by loop clauses are loop-local. They are removed when the
loop exits, and ordinary object lifetime rules apply to clause and body
objects. See [Labels and `GOTO`](labels-and-goto.md) for label-block and
unstructured-control rules.

## Clause constraints

The compiler rejects these combinations:

| Invalid form | Rule |
| --- | --- |
| A repeated clause | Every clause may occur at most once. |
| Sequence without `FROM` or `VALUE` | Both clauses are required. |
| Sequence with both or neither `TO` and `UNTIL` | Exactly one bound form is required. |
| Sequence with `INIT`, `EVAL`, `TEST`, `POSTTEST`, or `STEP` | Sequence loops have fixed initialization, test, and step phases. |
| Iterator clauses with `FROM`, `TO`, or `UNTIL` | Iterator and sequence forms are distinct. |
| `ITEM` with `INDEX` or `VALUE` | Direct-item and projected bindings are distinct. |
| `BY` with `STEP` | Only one advancement mechanism may be selected. |
| `END` with `LIMIT` | Only one explicit boundary comparison may be selected. |
| `IN` with `START`, `END`, or `LIMIT` | `IN` supplies its own iterator and boundary. |
| Iterator form without `IN` or `START` | An initial iterator is required. |
| Reused iterator binding names | `ITER`, `ITEM`, `INDEX`, and `VALUE` names must be distinct. |

