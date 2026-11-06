# Thread-pool coroutines

The standard module supplies asynchronous result ownership and a worker pool for
coroutines, recursive calls, and synchronous functions launched with `GO`.

```quxlang
IMPORT std;

::increment FUNCTION(@value I32): I32 { RETURN value + 1; }

::sum COROUTINE(@depth I32): I32
{
  IF (depth == 0) { RETURN 1; }
  VAR independent AUTO := GO increment(@value depth);
  VAR recursive I32 := sum(@depth depth - 1);
  RETURN recursive + AWAIT(independent);
}

::example FUNCTION(): I32
{
  VAR runtime std::thread_pool_cort_runtime:(@workers 2);
  VAR result AUTO := ON(runtime) RUN sum(@depth 4);
  RETURN result.get();
}
```

## Launch and controller protocol

`ON(runtime) RUN operation(...)` selects a coroutine through
`runtime.CO_RUN(@COROUTINE binding, @ARGUMENTS arguments)` and returns
`std::future#T`, where `T` is the coroutine's declared return type.

Within a scheduled coroutine, `GO operation(...)` invokes the controller's
`CO_GO` hook and returns its result directly. The standard controller returns
`std::future#T` for both synchronous functions and coroutines. A `VOID` operation
returns `std::future#VOID`. `GO` evaluates and captures arguments before returning;
it does not implicitly await completion.

The hooks receive the complete callable binding before overload selection. The
standard pool policy selects coroutine overloads when the binding contains a
coroutine. A function-only binding is selected through ordinary function overload
resolution. Explicit specialization selects the declaration before launch.

`std::thread_pool_cort_controller#T` connects an invocation to its pool, result
producer, and notification target. Its `CO_CALL_CORT` hook returns an eager
`std::task#U` for the called coroutine's return type `U`. The ordinary coroutine
call protocol awaits that task. Each recursive invocation has independent
allocated storage; the caller suspends while its child is pending. One worker can
therefore execute recursive calls without blocking on its own queued work.

`CO_CALL_CTOR` and `CO_CALL_DTOR` use the same scheduled invocation protocol.
`CO_RETURN` stages a value, and `CO_EXCEPTION` stages an exception. Publication
occurs after the frame reports terminal completion, including local and argument
cleanup. A cleanup exception replaces a previously staged return value.

`AWAIT(std::cort_yield())` queues another activation of the current invocation.
The pool returns to its ready queue before that activation starts.

## Futures, promises, and tasks

| Type | Ownership and operations |
| --- | --- |
| `std::future#T` | Move-only ownership of one consumer. `valid()`, `is_ready()`, `wait()`, `get()`, and the native awaiter protocol are available. |
| `std::promise#T` | Move-only ownership of one producer. `get_future()` acquires the consumer; `set_value(@value value)` or `set_exception(@error exception)` publishes its result. |
| `std::promise#VOID` | `set_value()` publishes successful completion without a value. |
| `std::task#T` | Move-only ownership of an eagerly scheduled result, with the same readiness, wait, retrieval, and await operations as its future. |
| `std::broken_promise` | Exception delivered when an incomplete producer is destroyed or replaced through move assignment. |

A future is consumed once through `get()` or `AWAIT`. Consumption leaves it
invalid, including when the producer's exception is rethrown. Moving transfers
ownership and leaves the source invalid. A default-constructed future is invalid.
Readiness, waiting, and retrieval require a valid future. `get_future()` is called
once per promise, and a producer publishes at most one result. The promise and
its consumer can operate on different threads. Operations on the same future or
the same promise object require external synchronization.

`get()` and `wait()` can block the calling thread. A pool coroutine uses `AWAIT`
for pending results so its worker can execute other invocations. The awaiter
checks readiness and registers its continuation under the result-state mutex.
Producer completion either observes that registration or causes registration to
decline suspension, preventing a lost notification.

Destroying a future or task releases consumer ownership without waiting for the
producer. A scheduled invocation remains owned by its pool until execution and
cleanup finish. The pool does not expose cancellation of submitted work.

## Pool and notification lifetime

`std::thread_pool_cort_runtime(@workers count)` starts `count` workers. The default
is four; the count must be positive. `post(@operation callable)` owns and queues a
callable whose zero-argument call operator completes synchronously without
throwing. `drain()` waits for every submitted invocation and its cleanup.
Destruction drains work, stops idle workers, and joins all worker threads.

`drain()` and pool destruction run outside that pool's workers. External
submission cannot overlap destruction. Borrowed arguments and receivers remain
alive until their invocations finish. Passing a temporary to a reference
parameter does not extend its lifetime. A coroutine waiting on an external promise
can keep `drain()` pending until that producer completes or is abandoned.

`std::async_continuation` contains a `MUT->VOID` context and a notification
procedure that completes without throwing. The controller's `continuation()`
returns its stable continuation.
An awaiter retains that target while registered and delivers one notification
when the awaited operation completes. The pool records a notification received
during an active drive and queues the next activation only after that drive
returns. An invocation never has two simultaneous drivers.

The pool and synchronization operations require native threading support. They
are not constant-evaluation operations. Coroutine control objects and function
captures use typed `VOID` pointer round trips; the library does not recover an
invocation from a contained object's address.

## Callable capture operations

`BIND_CALL function(arguments...)` selects a synchronous function and returns a
composite with `CALLEE` and `ARGUMENTS` fields. `CALLEE` identifies the selected
specialization. `ARGUMENTS` owns adapted value parameters and retains reference
parameters. Argument conversions and default expressions run during capture.
The selected function body runs when the stored call is invoked:

```quxlang
VAR bound AUTO := BIND_CALL increment(@value 41);
VAR result I32 := APPLY COMPOSITE_FORWARD(COMPOSITE_FIELD_GET(bound, "ARGUMENTS"))
  TO COMPOSITE_FIELD_GET(bound, "CALLEE");
```

A captured call that moves owned arguments is consumed once. Borrowed arguments
must outlive invocation. Coroutine targets and construction or destruction
destinations are rejected. The selected function's `RETURN_TYPE` metadata names
its return type. This metadata is available for both function and coroutine
specializations.

`HAS_COROUTINE_OVERLOAD(binding)` is a constant `BOOL` indicating whether the
callable binding contains a coroutine declaration. It does not select an overload
or prove that any overload accepts a particular argument list. `IS_FUNCTUM_CALLABLE_WITH(binding, @name Type, % [Type, ...])` tests invocation
viability using explicit argument types. The argument list supports
`COMPOSITE_UNPACK(CompositeType)` and can be omitted for a zero-argument call.
Overload resolution applies argument adaptation, default parameters, and declaration
constraints. A missing viable overload produces `FALSE`; ambiguous overloads and
malformed type arguments produce compilation errors. The query does not execute
the function or evaluate default argument values.

The pool controller uses `IS_COROUTINE_CALLABLE_WITH` with `@CONTROLLER AUTO` to test coroutine viability for
the actual argument types. A viable coroutine overload takes precedence. If no
coroutine overload is viable, the controller captures a synchronous call.
