# Overview of Lambdas

A lambda begins with `-<`. It may declare captures, parameters, a return type,
and either an expression or block body.

## Expression bodies

```quxlang
VAR source I32 := 4;

AUTO implicit_capture := -< = source;
AUTO reference_capture := -< [source] = source;
AUTO value_capture := -< [=source] = source;
AUTO no_capture := -< [] :I32 = 7;
```

The `=` introduces the lambda's returned expression; it is not assignment.

## Block bodies and parameters

```quxlang
AUTO limit_to_max := -< (@value I32, @maximum I32): I32
{
  IF (value > maximum)
  {
    RETURN maximum;
  }
  RETURN value;
};

ASSERT(limit_to_max(@value 7, @maximum 5) == 5);
```

Lambda parameters use the same named and positional syntax as functions. Named
parameters are preferred for ordinary lambda interfaces; positional parameters
remain available when position is intentional. A return type is optional when
it can be deduced from the body.

## Capture lists

- Omitting a capture list permits implicit reference capture.
- `[name]` explicitly captures `name` by reference.
- `[=name]` captures `name` by value.
- `[]` rejects any use of an uncaptured enclosing runtime local.

Capture discovery follows statically reachable paths. A name used only inside a
discarded `STATIC_IF` branch does not force an invalid capture.

Lambdas are function values and can be passed through `AUTO(name)` parameters.
See [Procedure pointers and function values](procedure-pointers-and-function-values.md).

## Reference

See the [Lambdas Reference](../reference/lambdas.md) for the complete
language rules, constraints, and technical edge cases.
