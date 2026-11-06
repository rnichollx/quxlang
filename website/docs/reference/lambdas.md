# Lambdas

A lambda is an unnamed callable value introduced by `-<`. It may capture
enclosing locals, declare call parameters and a return type, and use either a
block body or a returned-expression body.

## Grammar

```text
-< [ FUNCTION ] [ capture-list ] [ ( parameters ) ] [ qualifiers ] [ : return-type ] block
-< [ FUNCTION ] [ capture-list ] [ ( parameters ) ] [ qualifiers ] [ : return-type ] = expression
```

Every component after `-<` is optional except the body. The opening `[` of a
capture list distinguishes it from a parameter list, which starts with `(`.
Qualifiers are `NOEXCEPT`, `CANCELLABLE`, and `CANCELLABLE(INSTANT)`.
`NOEXCEPT` and a cancellation qualifier may appear in either order. Each may
appear at most once, and `FUNCTION` forbids cancellation qualifiers.

## Expression bodies

`= expression` creates a body that returns the expression:

```quxlang
AUTO seven := -< :I32 = 7;
ASSERT(seven() == 7);
```

The `=` belongs to the lambda syntax; it is not assignment. When the return type
is omitted, the compiler deduces a decayed value result from the body.

## Block bodies

A block uses ordinary function statements and return rules:

```quxlang
AUTO clamp := -< (@value I32, @maximum I32): I32
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

## Synchronous and coroutine invocation

A plain lambda provides both a synchronous and a coroutine `OPERATOR()`
overload. The enclosing invocation context and controller select the overload
through the ordinary function and coroutine call rules. `ON(controller) RUN`
passes the lambda binding and arguments to the controller's `CO_RUN` hook.
An ordinary call inside a coroutine passes the mixed binding to `CO_CALL_CORT`,
which controls selection. `GO` passes the binding to `CO_GO` and returns that
hook's result.

```quxlang
AUTO increment := -< (@value I32) = value + 1;
ASSERT(increment(@value 41) == 42);

VAR runtime std::thread_pool_cort_runtime:(@workers 1);
VAR pending := ON(runtime) RUN increment(@value 41);
ASSERT(pending.get() == 42);
```

`-< FUNCTION` provides only the synchronous overload:

```quxlang
AUTO increment := -< FUNCTION (@value I32) = value + 1;
```

A coroutine invocation supports `AWAIT` and `YIELD`. The body is checked for the
selected invocation mode. An instantiated `AWAIT` or `YIELD` requires coroutine
invocation even when the selected awaiter would complete synchronously. A lambda
with no explicit return type uses the normal body-based deduction rules for the
selected specialization. A controller that requires the declared return type
before choosing an execution controller may require an explicit return type when
deduction depends on that controller.

The coroutine overload defaults to `@CONTROLLER AUTO`. An explicit
`@CONTROLLER` parameter constrains its execution controller, and an optional
local parameter name names that controller in the body. It cannot have a
default argument. The controller parameter belongs only to the coroutine
overload. A synchronous invocation cannot use that parameter. During coroutine
invocation, `CONTROLLER` in a plain lambda denotes its own execution controller.
An enclosing controller can be captured through an ordinary local alias.

`CANCELLABLE` and `CANCELLABLE(INSTANT)` set the coroutine overload's cancellation
contract. `NOEXCEPT` applies to both invocation modes.

The coroutine frame borrows the closure receiver. The closure and every object
captured by reference must be alive at their original addresses until the
invocation completes or frame destruction finishes. Value captures belong to
the closure and are shared by invocations of that closure. Launching a coroutine
does not extend the closure's lifetime.

## Implicit reference capture

When the capture list is omitted, an enclosing runtime local referenced by the
lambda is captured by reference:

```quxlang
VAR value I32 := 4;
AUTO read := -< = value;

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
AUTO by_reference := -< [value] = value;
AUTO by_value := -< [=value] = value;
AUTO captures_nothing := -< [] :I32 = 7;
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
AUTO saved := -< [=source] = source;
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
AUTO outer := -< = (-< = value);
ASSERT(outer()() == 5);
```

## Static path sensitivity

Capture analysis follows the branch selected by compile-time control flow. A
name used only in a discarded `STATIC_IF` branch is not required and does not
make the closure invalid:

```quxlang
VAR value I32 := 6;
AUTO selected := -<
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

Capture analysis includes ordinary runtime branches and their referenced locals,
including branches with constant conditions.

## Callable value and identity

Each lambda expression has its own compiler-generated closure type with the
`OPERATOR()` overloads permitted by its invocation modes. Captures are the
closure's stored fields. The lambda can be stored with `AUTO`, copied or moved
when its captured fields permit it, and passed through a compatible function
value parameter.

See [Procedure Pointers and Function Values](procedure-pointers-and-function-values.md),
[Call Arguments](call-arguments.md), and [Functions](functions-and-parameters.md).

## Initialization parameters

A lambda may declare `@INITVAL NVALUE[T]` and initialize its destination with an
`INIT` statement. `INIT` continues execution. Ordinary `VOID` fallthrough and
`RETURN;` complete the call after exit cleanup. The same initialization and
forwarding rules apply to ordinary functions and callable objects; see
[Initialization parameters](functions-and-parameters.md#initialization-parameters).

The `NEW T COUNT(n) INDEX(i) DO { ... }` form lowers to an implicit lambda with an
additional `@INDEX:i SZ` parameter. Omitting `INDEX` omits that parameter. The body
uses ordinary capture and exception semantics. An unlabeled `CONTINUE;` outside
a nested runtime loop completes the implicit initializer body; fallthrough also
completes it. `RETURN` is reserved for explicit functions and lambdas. A nested
explicit lambda retains its own return rules. An uninitialized destination
cannot itself be captured by a nested lambda.
