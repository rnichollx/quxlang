# Static Evaluation and Runtime Selection

Quxlang uses ordinary source forms for compile-time execution and explicit
statements for code that differs between evaluation modes.

## Static selection

```quxlang
STATIC_IF(ARCH_IS_X64)
{
  selected := 64;
}
STATIC_ELSE
{
  selected := 32;
}

VAR chosen I32 := STATIC_CHOOSE(ARCH_IS_X64, 64, 32);
```

The unselected branch of `STATIC_IF` is discarded before its body needs to be a
valid runtime path. `STATIC_CHOOSE(condition, true_expression,
false_expression)` is the expression form.

## Static loops and mutation

```quxlang
VAR result I32 := 0;
STATIC_VAR index U64 := 0;

STATIC_WHILE(index < 3)
{
  result += index AS I32;
  STATIC_EVAL index++;
}

ASSERT(SNAPSHOT(index) == 3);
```

- `STATIC_VAR` declares expansion state inside a function.
- `STATIC_EVAL expression;` mutates or evaluates that static state.
- `STATIC_WHILE` repeats expansion.
- `SNAPSHOT(name)` materializes the expanded value for ordinary code.

## Runtime-mode selection

```quxlang
RUNTIME CONSTEXPR
{
  result := 1;
}
ELSE
{
  result := 2;
}

RUNTIME NATIVE
{
  native_only_operation();
}
ELSE
{
  portable_operation();
}
```

`RUNTIME CONSTEXPR` selects its first block during static execution and its
`ELSE` block during runtime execution. `RUNTIME NATIVE` selects its first block
for native execution and the alternative for non-native backends.

Target predicates are listed on [Availability and targets](availability-and-targets.md).

