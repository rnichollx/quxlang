# Conditional Statements

`IF` and `UNLESS` select a runtime block from a condition. `IF` executes its
first block when the condition is true; `UNLESS` executes its first block when
the condition is false.

```quxlang
IF (temperature < minimum)
{
  temperature := minimum;
}

UNLESS (ready)
{
  initialize();
}
```

## Conditions

The parenthesized expression must have type `BOOL` or be implicitly convertible
to `BOOL`. Quxlang evaluates it once before selecting a branch. Use an explicit
booliation expression such as `pointer??` or `pointer?!` when testing the
affirmative or empty state of a pointer or another booliable type.

```quxlang
IF (buffer??)
{
  consume(buffer->);
}
```

`UNLESS(condition)` inverts branch selection; it does not change how the
condition expression itself is evaluated.

## `ELSE` chains

`ELSE` supplies the alternative block. It may be followed by another `IF` or
`UNLESS`, producing one source-level chain:

```quxlang
IF (value < 0)
{
  category := -1;
}
ELSE IF (value == 0)
{
  category := 0;
}
ELSE UNLESS (value < maximum)
{
  category := 2;
}
ELSE
{
  category := 1;
}
```

Conditions are evaluated from left to right until one block is selected. Later
conditions and all unselected blocks do not execute.

An `IF` chain uses `ELSE`, not `STATIC_ELSE`. Compile-time branches use the
separate `STATIC_IF` and `STATIC_ELSE` syntax documented under
[Compile-Time Evaluation](compile-time-evaluation.md).

## Branch scope and values

Each branch is a function block. Variables declared in that block are local to
the block, and their lifetimes end when control leaves it. A name needed after
the chain must be declared before the `IF` and assigned by the selected branch:

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
use(magnitude);
```

The compiler converges the object state from all branches at the statement
following the chain. Each reachable path must therefore leave later-used
objects in a compatible lifetime state.

## Relationship to other selection forms

- `STATIC_IF` selects during compile-time generation and discards the
  unselected source path.
- `MATCH` dispatches on a `UNION` or `VARIANT` alternative.
- `INCLUDE_IF` conditionally includes a declaration in the active source
  configuration.

Those forms have different evaluation times and cannot be substituted merely
by changing the keyword on an `IF` statement.

