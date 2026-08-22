# Overview of Variables

Variables give names to mutable values. Use `VAR` for local working state,
program-wide state, and structure data members.

## Local variables

Inside a function, write the variable name followed by its type:

```quxlang
::sum_first_three FUNCTION(): I32
{
  VAR values [3]I32 :[4, 7, 9];
  VAR total I32 := 0;

  FOR VALUE(value) IN(values) LOOP
  {
    total := total + value;
  }

  RETURN total;
}
```

`values` is initialized from a positional sequence. `total` is copy-initialized
from the expression after `:=`, then changed with assignment inside the loop.

## Default initialization

An initializer may be omitted:

```quxlang
VAR attempts I32;
VAR finished BOOL;
VAR destination MUT->BYTE;
```

This calls the type's no-argument constructor. Built-in integers begin at zero,
`BOOL` begins as `FALSE`, and pointers begin null. A user-defined type must have
an applicable default constructor.

## Constructor arguments

Use `:(...)` to pass named or positional constructor arguments:

```quxlang
::coordinate STRUCT
{
  .x VAR I32;
  .y VAR I32;

  .CONSTRUCTOR FUNCTION(@x I32, @y I32)
  {
    .x := x;
    .y := y;
  }
}

::coordinate_example FUNCTION(): I32
{
  VAR point coordinate :(@x 3, @y 5);
  RETURN point.x + point.y;
}
```

Structure members use `.name VAR Type;`. Their owning constructor assigns the
members.

## Global variables

At namespace scope, the name comes before `VAR`:

```quxlang
::request_count VAR U64 := 0;

::record_request FUNCTION()
{
  request_count++;
}
```

Every access names the same program-wide mutable object. If multiple threads
modify it, use the synchronization appropriate for the program. For an
independent instance in each thread, use [`PER_THREAD VAR`](thread-local-variables.md).

## References and deduced variables

A reference variable binds to an existing object:

```quxlang
VAR value I32 := 10;
VAR alias MUT& I32 := value;
alias := 12;
ASSERT(value == 12);
```

A local `AUTO` variable can deduce its type from one `:=` initializer. Prefer an
explicit type when it makes the code's ownership or numeric width clearer.

## Reference

For every initialization form, scope rule, and storage restriction, see the
[Variables Reference](../../reference/variables.md).
