# Overview of Exception Handling

Exceptions report a failure to a caller that can handle it. A program can throw
an application-specific type:

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

The caller selects a handler by the exception type:

```quxlang
VAR accepted BOOL := FALSE;

TRY
{
  require_positive(@value 0);
  accepted := TRUE;
}
CATCH error CONST& input_error
{
  ASSERT(error.value == 0);
}

ASSERT(accepted == FALSE);
```

The exception stores the thrown value. The catch binds a reference to that
value, allowing a structured error to carry context without copying it into
each handler.

## Automatic cleanup

When an exception leaves a scope, completed local objects are destroyed and
`DEFER` actions run in reverse order:

```quxlang
TRY
{
  VAR resource resource_owner := acquire_resource();
  DEFER record_attempt();
  perform_operation(@resource resource);
}
CATCH error CONST& operation_error
{
  report_failure(@error error);
}
```

Exceptions support failure propagation through several function calls while
preserving scope-based cleanup. Return values, optionals, and variants represent
expected outcomes handled directly by the caller.

Throwing an exception unwinds each exited scope and runs its cleanup. The work
performed during unwinding depends on the scopes and objects crossed.
`NOEXCEPT` marks a boundary that must handle exceptions before they escape.

The language also supports rethrowing, `EXCEPTION_PTR` handles that preserve an
exception, a catch-all form, and a dedicated out-of-memory exception. The
[Exception Handling Reference](../reference/exceptions.md) specifies matching
order, lifetime, nested handlers, termination behavior, and target support.
