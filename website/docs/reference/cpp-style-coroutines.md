# C++-style coroutines

`std::cpp_style_coroutine_base` launches Quxlang coroutines through C++-style
promise and awaiter operations. The declared return type supplies a nested
`promise_type`. Each invocation allocates its promise, controller, and concrete
frame at stable addresses. Promise types are ordinary unrelated structures.

```quxlang
VAR scheduler std::cpp_style_coroutine_base;
VAR result AUTO := ON(scheduler) RUN produce(@input 41);
```

The scheduler can also be a base subobject of an application scheduler. Its
`CO_RUN` operation receives the complete binding and original argument pack.
Member receivers, argument conversion, and default arguments follow ordinary
coroutine specialization and frame initialization.

## Promise operations

| Operation | Purpose |
| --- | --- |
| `get_return_object(@handle std::cpp_coroutine_handle#promise_type)` | Receives the invocation connection and produces the declared return object before initial suspension. |
| `initial_suspend()` | Supplies the awaitable that controls when the body starts. |
| `final_suspend()` | Supplies the awaitable used after body completion and cleanup. |
| `return_value(@value operand)` | Accepts the original operand of a value return. |
| `return_void()` | Handles bare returns, `VOID` operands, and reachable fallthrough. |
| `yield_value(@value operand)` | Supplies the awaitable for a yield. |
| `unhandled_exception()` | Receives an unhandled body exception through `CURRENT_EXCEPTION()`. |
| `await_transform(@value operand)` | Optionally transforms an ordinary await operand. |

The `handle` argument identifies the current invocation. Its `promise()` operation
returns a reference to the receiving promise. A promise can pass the handle to
its return object's constructor:

```quxlang
.get_return_object FUNCTION(@handle std::cpp_coroutine_handle#promise_type) MUT: task
{
  RETURN task(@handle handle);
}
```

The promise is default-constructed. Initial-suspension setup and final-suspension
operations must not throw. An exception from the initial awaiter's
`await_resume` reaches `unhandled_exception`. Failure while constructing the
return object releases the invocation and its captured arguments.

`RETURN` and `YIELD` preserve the operand's type and reference category when
selecting the promise operation. Initial suspension, final suspension, yield,
and internal completion operations bypass `await_transform`.

A coroutine call inside the body returns the child's return object. The
adapter selects a new promise and controller from the child's declared return
type. An explicit `AWAIT` awaits that return object when the object supplies an
awaiter protocol. `GO` also returns the launched child's return object.

## Awaiter operations

An awaitable can supply `operator_co_await()` to produce its awaiter. Otherwise,
the awaitable itself supplies the following operations:

| Operation | Result |
| --- | --- |
| `await_ready()` | `BOOL` indicating immediate readiness. |
| `await_suspend(@handle std::cpp_coroutine_handle#promise_type)` | `VOID`, `BOOL`, or a coroutine handle. |
| `await_resume()` | The await expression's result, including references. |

A `VOID` suspension result suspends the invocation. A `FALSE` Boolean result
continues within the same activation. A handle result transfers execution after
the active driver returns. Successive transfers use an iterative driver loop.
External scheduling must satisfy Quxlang's non-reentrant frame-drive contract.

`std::cpp_suspend_always` and `std::cpp_suspend_never` provide the corresponding
ready/suspend/resume protocols. Native Quxlang awaiters exposing `AWAIT_READY`
can also pass directly through the controller.

## Coroutine handles

`std::cpp_coroutine_handle#P` is a copyable, non-owning reference to one
invocation. The template parameter is the promise type. `#VOID` erases promise
access and accepts conversion from a typed handle.

| Operation | Contract |
| --- | --- |
| Default construction or construction from `NULL` | Produces an empty handle. |
| Assignment from `NULL` | Clears the reference without destroying the invocation. |
| `handle??` | Reports whether the handle is nonempty. |
| `address()` | Exports invocation identity as `MUT->VOID`. |
| `from_address(@address MUT->VOID)` | Reconstructs a handle from an exported identity. |
| `promise()` | Returns `MUT& P`. Unavailable for `VOID`. |
| `done()` | Reports final suspension. |
| `resume()` or `handle()` | Resumes a suspended invocation. |
| `destroy()` | Destroys a suspended invocation and invalidates all referring handles. |
| `==` | Compares invocation identity across promise types. |

`from_address` accepts an identity returned by `address()`, including the null
identity of an empty handle. The destination promise type must match the original
invocation's promise type or be `VOID`.

`done`, `resume`, `destroy`, and `promise` require a live, nonempty handle.
`done`, `resume`, and `destroy` require a suspended invocation. Resuming an
invocation at final suspension is invalid. Destruction requires retirement of
external continuation registrations and synchronous cleanup. Destruction skips
body catch handlers and does not require a cancellation declaration. The
return object determines ownership policy; the handle itself does not own the
allocation.

The adapter passes a typed invocation handle to `get_return_object`. The promise
can copy this handle into its return object or retain it for subsequent operations.
Invocation and promise pointers retain their types through `VOID` pointer erasure
and `REINTERPRET` conversion back to the original types.

Final suspension retains the allocation until `destroy`. If final suspension
completes synchronously, the adapter releases the allocation immediately.
Quxlang frame completion releases its captured parameters before final
suspension. Handles use the Quxlang adapter representation and do not provide
interoperability with a foreign C++ coroutine ABI.
