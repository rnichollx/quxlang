# Lambdas

A lambda is an unnamed callable value introduced by `-<`. It may capture
enclosing locals, declare call parameters and a return type, and use either a
block body or a returned-expression body.

## Grammar

```text
-< [ capture-list ] [ ( parameters ) ] [ : return-type ] block
-< [ capture-list ] [ ( parameters ) ] [ : return-type ] = expression
```

Every component after `-<` is optional except the body. Because `[` starts a
capture list and `(` starts parameters, the forms remain unambiguous.

## Expression bodies

`= expression` creates a body that returns the expression:

```quxlang
VAR seven AUTO := -< :I32 = 7;
ASSERT(seven() == 7);
```

The `=` belongs to the lambda syntax; it is not assignment. When the return type
is omitted, the compiler deduces a decayed value result from the body.

## Block bodies

A block uses ordinary function statements and return rules:

```quxlang
VAR clamp AUTO := -< (@value I32, @maximum I32): I32
{
  IF (value > maximum)
  {
    RETURN maximum;
  }
  RETURN value;
};

ASSERT(clamp(@value 7, @maximum 5) == 5);
```

Lambda parameters use the full function-parameter grammar, including named and
positional parameters, type deduction, packs, and defaults. Calls use the same
argument syntax as other callable values. An empty block with no explicit
return type deduces `VOID`.

## Implicit reference capture

When the capture list is omitted, an enclosing runtime local referenced by the
lambda is captured by reference:

```quxlang
VAR value I32 := 4;
VAR read AUTO := -< = value;

value := 9;
ASSERT(read() == 9);
```

Only referenced locals become closure fields. Function symbols, types, and
compile-time names do not need runtime capture. A reference capture refers to
the original object, so the object must remain alive and stationary whenever
the lambda or a copy of it is called.

## Explicit capture lists

An explicit list is both a capture declaration and an allowlist:

```quxlang
VAR value I32 := 4;
VAR by_reference AUTO := -< [value] = value;
VAR by_value AUTO := -< [=value] = value;
VAR captures_nothing AUTO := -< [] :I32 = 7;
```

- `name` captures that local by reference;
- `=name` captures its value in the closure;
- `[]` allows no enclosing runtime locals.

A name in an explicit list must be available at the lambda expression. If the
body uses an outer local not listed, the lambda is rejected. By-value capture
stores an independent object initialized when the lambda value is created;
later mutation of the source does not change the captured value.

```quxlang
VAR source I32 := 4;
VAR saved AUTO := -< [=source] = source;
source := 9;
ASSERT(saved() == 4);
```

## Capture access and nesting

The captured reference preserves the source's readable or mutable access. A
`WRITE&` source is retained as mutable access because a stored closure may need
to use it after initialization. A by-value capture has the value type with any
reference layer removed.

A nested lambda can use a local from an outer function. Its capture requirement
bubbles through intervening lambdas so each closure environment can provide the
name to the next nested closure.

```quxlang
VAR value I32 := 5;
VAR outer AUTO := -< = (-< = value);
ASSERT(outer()() == 5);
```

## Static path sensitivity

Capture analysis follows the branch selected by compile-time control flow. A
name used only in a discarded `STATIC_IF` branch is not required and does not
make the closure invalid:

```quxlang
VAR value I32 := 6;
VAR selected AUTO := -<
{
  STATIC_IF(FALSE)
  {
    RETURN missing_name;
  }
  STATIC_ELSE
  {
    RETURN value;
  }
};
```

Ordinary runtime branches remain part of the body and therefore contribute
their referenced locals even when a condition happens to be constant.

## Callable value and identity

Each lambda expression has its own compiler-generated closure type with an
`OPERATOR()` corresponding to its parameters and result. Captures are the
closure's stored fields. The lambda can be stored with `AUTO`, copied or moved
when its captured fields permit it, and passed through a compatible function
value parameter.

See [Procedure Pointers and Function Values](procedure-pointers-and-function-values.md),
[Call Arguments](call-arguments.md), and [Functions](functions-and-parameters.md).
