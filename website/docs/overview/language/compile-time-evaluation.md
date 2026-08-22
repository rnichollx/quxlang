# Overview of Compile-Time Evaluation

Compile-time evaluation lets a function use values and control flow while its
code is being generated. It is useful for selecting target-specific source and
expanding a fixed amount of repeated code.

## Selecting source with `STATIC_IF`

`STATIC_IF` evaluates its condition at compile time and keeps only the selected
body:

```quxlang
::register_width FUNCTION(): I32
{
  VAR result I32 := 0;

  STATIC_IF(ARCH_IS_X64)
  {
    result := 64;
  }
  STATIC_ELSE
  {
    result := 32;
  }

  RETURN result;
}
```

Use `STATIC_ELSE`, not the ordinary runtime `ELSE`. Because the unselected body
is not generated, a branch can refer to declarations available only on its
selected target.

For a value-sized choice, use `STATIC_CHOOSE`:

```quxlang
VAR width I32 := STATIC_CHOOSE(ARCH_IS_X64, 64, 32);
```

Only the chosen expression is generated.

## Compile-time variables and loops

`STATIC_VAR` holds mutable state during generation. Change it with
`STATIC_EVAL`, and repeat generation with `STATIC_WHILE`:

```quxlang
::add_three FUNCTION(@value MUT& I32)
{
  STATIC_VAR index U64 := 0;

  STATIC_WHILE(index < 3)
  {
    value++;
    STATIC_EVAL index++;
  }
}
```

The generated function contains three increments. This is not a runtime loop:
`STATIC_WHILE` repeats generation, while an ordinary `WHILE` emits one loop
whose iterations happen when the program runs.

Use function-local `STATIC` for a generation-time value that must not change:

```quxlang
STATIC count U64 := 3;
```

## Capturing the result

Runtime code reads the current value of a `STATIC_VAR` through `SNAPSHOT`:

```quxlang
STATIC_VAR count I32 := 1;
VAR before I32 := SNAPSHOT(count);
STATIC_EVAL count++;
VAR after I32 := SNAPSHOT(count);

ASSERT(before == 1);
ASSERT(after == 2);
```

Each call captures the value at that point. Earlier captures are not changed by
later compile-time mutations.

For scope, mutability, branch-generation, snapshot, and error rules, see the
[Compile-Time Evaluation Reference](../../reference/compile-time-evaluation.md).
