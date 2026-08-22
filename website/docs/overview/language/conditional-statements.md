# Overview of Conditional Statements

`IF` executes a block when a condition is true. `UNLESS` is its negative form
and executes a block when the condition is false.

```quxlang
IF (score >= passing_score)
{
  passed := TRUE;
}

UNLESS (initialized)
{
  initialize();
}
```

Conditions use `BOOL`. For a pointer or another type with an affirmative state,
write the booliation test explicitly:

```quxlang
IF (pointer??)
{
  use(pointer->);
}
```

## Choosing among alternatives

Use `ELSE IF`, `ELSE UNLESS`, and a final `ELSE` to form a chain:

```quxlang
IF (value < 0)
{
  category := -1;
}
ELSE IF (value == 0)
{
  category := 0;
}
ELSE
{
  category := 1;
}
```

Conditions are checked from left to right. Only the first selected branch
executes. Variables declared inside a branch are local to that branch, so
declare an output before the chain when later code needs it.

```quxlang
VAR magnitude I32;
IF (value < 0)
{
  magnitude := 0 - value;
}
ELSE
{
  magnitude := value;
}
```

## Complete technical rules

See the [Conditional Statements Reference](../../reference/conditional-statements.md)
for conversion to `BOOL`, exact chain evaluation, branch scope, lifetime-state
convergence, and the distinction from `STATIC_IF`, `MATCH`, and `INCLUDE_IF`.

