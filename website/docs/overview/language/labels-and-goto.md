# Overview of Labels and GOTO

## Labeled loops

```quxlang
FOR :outer VALUE(i) FROM(0 AS I32) TO(4) LOOP
{
  WHILE :inner (condition)
  {
    CONTINUE :outer;
  }
  BREAK :outer;
};
```

`BREAK` and `CONTINUE` accept an optional `:label`. An unlabeled statement
targets its nearest valid enclosing construct.

## Labeled blocks

```quxlang
LABEL :done
{
  BREAK :done;
}
```

A labeled block is breakable even though it is not a loop.

## `GOTO`

```quxlang
GOTO :retry;
LABEL :retry;
```

`LABEL :name;` declares a statement target. `GOTO :name;` transfers control to
it within the function.

Object lifetime remains authoritative: a jump cannot enter a scope past a
required construction, bypass an initialization whose lifetime would become
active, or otherwise create an impossible lifetime state. Leaving scopes runs
the destruction required by the language's lifetime rules.

## Complete technical rules

See the [Labels and GOTO Reference](../../reference/labels-and-goto.md) for the complete
language rules, constraints, and technical edge cases.
