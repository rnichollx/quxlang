# Lambdas

A lambda begins with `-<`. It may declare captures, parameters, a return type,
and either an expression or block body.

## Expression bodies

```quxlang
VAR source I32 := 4;

VAR implicit_capture AUTO := -< = source;
VAR reference_capture AUTO := -< [source] = source;
VAR value_capture AUTO := -< [=source] = source;
VAR no_capture AUTO := -< [] :I32 = 7;
```

The `=` introduces the lambda's returned expression; it is not assignment.

## Block bodies and parameters

```quxlang
VAR source I32 := 4;

VAR add_source AUTO := -< (%value I32): I32
{
  RETURN value + source;
};

ASSERT(add_source(% [3]) == 7);
```

Lambda parameters use the same named and positional syntax as functions. A
return type is optional when it can be deduced from the body.

## Capture lists

- Omitting a capture list permits implicit reference capture.
- `[name]` explicitly captures `name` by reference.
- `[=name]` captures `name` by value.
- `[]` rejects any use of an uncaptured enclosing runtime local.

Capture discovery follows statically reachable paths. A name used only inside a
discarded `STATIC_IF` branch does not force an invalid capture.

Lambdas are function values and can be passed through `AUTO(name)` parameters.
See [Procedure pointers and function values](procedure-pointers-and-function-values.md).

