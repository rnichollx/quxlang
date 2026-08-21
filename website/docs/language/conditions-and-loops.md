# Conditions and Loops

## `IF` and `UNLESS`

```quxlang
IF (value < 0)
{
  value := 0 - value;
}
ELSE IF (value == 0)
{
  value := 1;
}
ELSE
{
  value++;
}

UNLESS (finished)
{
  work();
}
```

`UNLESS(condition)` is the negative form of `IF(condition)`. Chains may use
`ELSE IF`, `ELSE UNLESS`, and a final `ELSE`.

Conditions have type `BOOL`; Quxlang does not silently convert arbitrary values
to Boolean. Use `value??` or `value?!` where a type defines an affirmative state.

## `WHILE`

```quxlang
WHILE (index < count)
{
  index++;
}
```

A loop can be labeled for targeted `BREAK` or `CONTINUE`:

```quxlang
WHILE :searching (condition)
{
  BREAK :searching;
}
```

The more general clause-based loop is documented on [`FOR` clauses](for-clauses.md).
Compile-time conditions and loops are documented on
[Static evaluation](static-evaluation.md).

