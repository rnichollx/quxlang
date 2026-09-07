# Overview of Exception Handling

Use exceptions to report a failure to a caller that can handle it. `THROW`
leaves the current path, and `TRY` with `CATCH` selects a handler while cleaning
up the scopes being exited.

## Throw a value and catch a reference

```quxlang
VAR result I32 := 0;
TRY
{
  THROW I32(@OTHER 42);
}
CATCH error CONST& I32
{
  result := error;
}
ASSERT(result == 42);
```

The exception owns its value. A typed catch binds a `CONST&` or `MUT&`
reference to that value; it does not copy it into the handler. Give literals a
runtime type, as with `I32(@OTHER 42)` above.

There is no required exception base class. A structure can carry the details
that callers need:

```quxlang
::input_error STRUCT
{
  .value VAR I32;
}

::require_positive FUNCTION(@value I32)
{
  IF (value < 1)
  {
    VAR error input_error;
    error.value := value;
    THROW error;
  }
}
```

```quxlang
VAR rejected BOOL := FALSE;
TRY { require_positive(@value 0); }
CATCH error CONST& input_error { rejected := error.value == 0; }
ASSERT(rejected);
```

Handlers are tried in source order. Put a more specific handler before a
broader one. `CATCH DEFAULT { ... }` handles anything left over and must be
last. If no handler matches, the exception continues to an enclosing `TRY` or
a caller.

## Clean up before handling the failure

Local objects are destroyed when an exception exits their scopes. Use `DEFER`
for a scope-exit action:

```quxlang
VAR events I32 := 0;
TRY
{
  DEFER events := events * 10 + 1;
  DEFER events := events * 10 + 2;
  THROW I32(@OTHER 42);
}
CATCH error CONST& I32
{
  ASSERT(error == 42);
  ASSERT(events == 21);
}
```

Deferred actions run in reverse order when their scope ends, including normal
exit. Destructors and deferred actions must handle any exceptions they raise
internally: an exception escaping either terminates execution.

## Handle part of a failure and rethrow it

`RETHROW;` sends the same exception outward. A mutable catch can update its
payload first:

```quxlang
VAR result I32 := 0;
TRY
{
  TRY { THROW I32(@OTHER 42); }
  CATCH error MUT& I32
  {
    error := 73;
    RETHROW;
  }
}
CATCH error CONST& I32 { result := error; }
ASSERT(result == 73);
```

The rethrow belongs to the enclosing catch in the same callable. A separately
called function or lambda uses an exception handle to propagate a saved
exception instead.

## Keep an exception after its handler returns

`CURRENT_EXCEPTION()` returns an owning `EXCEPTION_PTR` for the currently
active handler. Saving it keeps the exception alive:

```quxlang
VAR saved EXCEPTION_PTR;
ASSERT((saved??) == FALSE);
TRY { THROW I32(@OTHER 42); }
CATCH error MUT& I32
{
  saved := CURRENT_EXCEPTION();
  error := 73;
}
ASSERT(saved??);
ASSERT((CURRENT_EXCEPTION()??) == FALSE);

VAR result I32 := 0;
TRY { THROW_EXCEPTION_PTR(@exception saved); }
CATCH error CONST& I32 { result := error; }
ASSERT(result == 73);
```

The postfix `??` operator tests whether the handle contains an exception.
Copying a handle shares ownership of the same exception; it does not copy the
payload. `THROW_EXCEPTION_PTR` requires a nonempty handle.

## Handle failure to allocate exception storage

`UNWIND_OUT_OF_MEMORY` is a special exception with no ordinary payload. Catch
it without a binding name or type:

```quxlang
VAR handled BOOL := FALSE;
TRY { THROW UNWIND_OUT_OF_MEMORY; }
CATCH UNWIND_OUT_OF_MEMORY
{
  VAR exception EXCEPTION_PTR := CURRENT_EXCEPTION();
  ASSERT(exception??);
  ASSERT(exception.IS_OUT_OF_MEMORY());
  handled := TRUE;
}
ASSERT(handled);
```

The runtime uses this sentinel when it cannot allocate exception storage. It
can also be thrown explicitly, as above. Typed object catches do not match it;
`CATCH DEFAULT` does. Keep recovery code able to run under memory pressure.

## Mark a nonthrowing boundary

`NOEXCEPT` promises that no exception escapes a callable. It can still handle
exceptions inside its body:

```quxlang
::recover_value FUNCTION() NOEXCEPT: I32
{
  TRY { THROW I32(@OTHER 42); }
  CATCH error CONST& I32 { RETURN error; }
  RETURN 0;
}
```

An exception that escapes `NOEXCEPT`, or reaches the end of the call stack
without a handler, terminates native execution. During constant evaluation it
is an execution failure. Failed assertions and `PANIC` are terminal failures;
`CATCH` does not recover from them.

## Reference

See the [Exception Handling Reference](../reference/exceptions.md) for exact
matching rules, nested-handler behavior, lifetime rules, and target support.
