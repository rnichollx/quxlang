# Coroutine execution

A `COROUTINE` declaration describes a suspended invocation whose frame storage
and execution controller are selected by its caller. Each coroutine/controller
specialization has a nominal `FRAME_TYPE`. The controller adapts awaitables, and
the concrete awaiter supplies the readiness, suspension, and result operations.

## Caller-owned frames

The following example uses caller-owned storage and explicit resumption. Its
awaiter registers no external notification; the enclosing function supplies the
authorized second activation.

```quxlang
LANGUAGE QUXLANG EN 0.0;

::awaiter STRUCT
{
  .ready VAR BOOL;
  .AWAIT_READY FUNCTION() CONST: BOOL { RETURN .ready; }
  .AWAIT_SUSPEND FUNCTION(@CONTROLLER MUT& controller, @FRAME MUT& AUTO): BOOL
  {
    RETURN TRUE;
  }
  .AWAIT_RESUME FUNCTION() {}
}

::controller STRUCT
{
  .CO_AWAIT FUNCTION(@ARG BOOL): awaiter
  {
    VAR result awaiter;
    result.ready := ARG;
    RETURN result;
  }
  .CO_RETURN FUNCTION(): BOOL { RETURN TRUE; }
  .CO_EXCEPTION FUNCTION(@ARG CONST& EXCEPTION_PTR): BOOL
  {
    THROW_EXCEPTION_PTR(ARG);
  }
}

::count COROUTINE(@VALUE:value MUT& I32)
{
  value += 1;
  AWAIT(FALSE);
  value += 1;
}

::example FUNCTION(): I32
{
  VAR execution controller;
  VAR value I32;
  VAR frame count#(@VALUE MUT& I32, @CONTROLLER controller)::FRAME_TYPE;
  frame.INITIALIZE(@VALUE value);
  TEST_ASSERT(value == 0);
  TEST_ASSERT(frame.RESUME(@CONTROLLER execution) == FALSE);
  TEST_ASSERT(value == 1);
  TEST_ASSERT(frame.RESUME(@CONTROLLER execution));
  RETURN value;
}
```

`INITIALIZE` captures parameters synchronously. The coroutine body and controller
hooks begin during the first drive. Failed capture rolls back synchronously and
leaves an empty frame. Parameter capture that requires asynchronous construction
or rollback is rejected.

`RESUME(@CONTROLLER MUT& C): BOOL` returns `FALSE` when execution suspends and
`TRUE` after terminal reporting and required cleanup finish. A terminal exception
can propagate with the frame already completed. Further drives of a completed
frame return `TRUE` without replaying controller hooks.

Completed frames can be initialized again after every registration from the
previous invocation has been retired. A frame retains its address throughout an
invocation. Copying and moving frame objects are rejected. Concurrent or reentrant
driving, driving an empty frame, and releasing an unfinished invocation are
invalid.

The controller remains external to the frame. The generated frame contains no
mandatory allocator, controller pointer, scheduler pointer, reference count, or
virtual dispatch table. Storage allocation belongs to an adapter. The standard
library supplies a borrowing `std::coroutine_view#descriptor` and an owning
`std::coroutine_handle#descriptor`; moving the owning handle preserves its frame's
address. A controller object referenced by a retained awaiter or registration
must remain alive until those references are retired.

## Descriptors and declarations

The completion type follows `:`, and defaults to `VOID`. An omitted controller
parameter introduces `@CONTROLLER AUTO`; an explicit controller parameter
constrains specialization. Descriptors expose the following members:

| Member | Meaning |
| --- | --- |
| `FRAME_TYPE` | Rooted storage for one concrete coroutine/controller specialization. |
| `CONTROLLER_TYPE` | The external controller type. |
| `RETURN_TYPE` | The coroutine's completion type. |
| `CANCELLATION_MODE` | `NONE`, `COOPERATIVE`, or `INSTANT`. |

Named and member coroutines are supported. Member frame initialization receives
the receiver through `@RECEIVER`. Coroutine lambdas, virtual coroutine dispatch,
and a foreign coroutine ABI are outside this interface. Ordinary virtual
destructor alternatives remain available independently of statically selected
coroutine destruction.

## Mixed function and coroutine overloads

A functum can contain both ordinary functions and coroutines with the same
ordinary parameters:

```quxlang
::increment FUNCTION(@VALUE MUT& I32) { VALUE += 1; }
::increment COROUTINE(@VALUE MUT& I32)
{
  AWAIT(FALSE);
  VALUE += 1;
}
```

A call in a coroutine first performs overload resolution with its implicit
`@CONTROLLER` argument. If that attempt finds no viable overload, resolution
retries with the original arguments. An explicitly supplied controller remains
part of those original arguments. Both attempts occur during compilation, and
argument expressions execute once. Ambiguity and errors in a selected overload
remain errors.

The selected declaration determines execution: ordinary functions execute
directly, and ordinary coroutine calls pass through `CO_CALL_CORT`. Ordinary
execution contexts perform lookup without an implicit controller. Descriptor specialization retains
the `coroutine#(...)::FRAME_TYPE` interface and can omit a controller whose type
is fixed by the coroutine declaration.

## Coroutine launch expressions

An explicit launch selects a scheduler or controller for one coroutine invocation:

```quxlang
VAR task AUTO := ON(scheduler) RUN work(@INPUT input);
```

The expression evaluates the controller before the target receiver and arguments.
The selected controller applies only to the target invocation. Calls within the
controller, receiver, and argument expressions retain their enclosing execution
context. The target must resolve to a coroutine. Its arguments are evaluated once
and prepared through ordinary argument adaptation before controller dispatch.

`CO_RUN(@COROUTINE descriptor, @ARGUMENTS arguments)` is a synchronous hook whose
result becomes the launch expression's result directly. The hook selects storage,
ownership, and the concrete child controller, then arranges execution according to
its scheduling policy. An ordinary function can use this expression without
becoming a coroutine.

Within a coroutine, `GO work(@INPUT input)` uses the enclosing controller's
`CO_GO(@COROUTINE descriptor, @ARGUMENTS arguments)` hook. The hook returns an
awaitable, which the calling coroutine awaits through its ordinary await protocol.
The launch awaitable can complete immediately or suspend the caller until the
controller's launch condition is satisfied. The awaited result becomes the `GO`
expression's result. A result that is itself awaitable requires a separate
`AWAIT` expression to await its completion.

Both hooks can specialize the selected coroutine with another controller type.
Rebinding preserves the selected declaration and must satisfy its constraints.
The adapter preserves frame storage and child-controller lifetimes until their
execution and continuation registrations have completed. Discarding a launch
result follows ordinary destruction rules.

## Awaitables and awaiters

`AWAIT(expression)` evaluates its operand once and passes it to
`controller.CO_AWAIT(@ARG awaitable)`. The returned awaiter can be an owned value
or a reference. Its concrete type determines these statically selected calls:

```text
awaiter.AWAIT_READY(): BOOL
awaiter.AWAIT_SUSPEND(@CONTROLLER MUT& C, @FRAME MUT& F): BOOL
awaiter.AWAIT_RESUME() -> result
awaiter.TRY_CANCEL(@CONTROLLER MUT& C, @FRAME MUT& F): BOOL
```

The compiler retains the operand and awaiter for their required lifetimes,
including suspension and cleanup. Passing the awaiter as a receiver does not
imply a copy or an allocation. An awaitable describes the requested work; its
awaiter carries the data needed while awaiting that work. Several suspension
points can use the same awaiter type. The frame's continuation identifier selects
the resumption location independently of that type.

When `AWAIT_READY` returns `TRUE`, execution calls `AWAIT_RESUME` directly.
Otherwise, the compiler saves the continuation before calling `AWAIT_SUSPEND`.
A `FALSE` suspension result continues inline and requires that no future
notification remain registered. A `TRUE` result returns from the driver. An
authorized subsequent drive calls `AWAIT_RESUME` without repeating readiness or
registration.

The controller and awaiter own synchronization and exactly-once notification
dispatch. A notification can be queued while a driver is active, but it cannot
reenter that drive. A throwing suspension hook must retire any independently
scheduled continuation before propagating.

## Controller hooks

Controller hooks are ordinary synchronous functions. Their returned awaitables
represent asynchronous work through the awaiter protocol.

```text
CO_CALL_CORT(@COROUTINE descriptor, @ARGUMENTS arguments) -> awaitable
CO_CALL_CTOR(@COROUTINE descriptor, @ARGUMENTS arguments,
             @DESTINATION borrowed_storage) -> awaitable
CO_CALL_DTOR(@COROUTINE descriptor, @ARGUMENTS arguments) -> awaitable
CO_RUN(@COROUTINE descriptor, @ARGUMENTS arguments) -> result
CO_GO(@COROUTINE descriptor, @ARGUMENTS arguments) -> awaitable
CO_AWAIT(@ARG awaitable) -> awaiter_or_reference
CO_YIELD(@ARG value) -> awaitable
CO_RETURN(@ARG value) -> awaitable
CO_RETURN() -> awaitable
CO_EXCEPTION(@ARG CONST& EXCEPTION_PTR) -> awaitable
CO_CANCELLED(@ARG MUT& frame) NOEXCEPT: VOID
```

Ordinary coroutine calls select `CO_CALL_CORT`, coroutine construction selects
`CO_CALL_CTOR`, and coroutine destruction selects `CO_CALL_DTOR`. Each invocation
implicitly awaits the returned value. The selected hook determines child-controller
adaptation, frame storage, and ownership. `CO_CALL_CTOR` additionally borrows typed
destination storage through `@DESTINATION`. Hook selection is static.

`CO_CALL_DTOR` accepts responsibility for mandatory destruction when
its invocation begins. Argument preparation completes before that transfer. The
hook may execute the child immediately or return an awaitable that owns the
remaining teardown. The hook must finish the accepted destruction before
propagating an exception; the caller does not invoke that destructor again.

Each `YIELD expression` independently selects `CO_YIELD` using the operand's
actual type and reference category. A declaration has no homogeneous yield type
or shared yield-value slot.

Return, yield, and exception-reporting awaiters complete with `VOID`. `CO_RETURN`
receives a ready completion value whose lifetime covers transfer and transfer
awaiter cleanup. Async-only values are borrowed by the synchronous hook. Failure
during later required cleanup enters exception reporting. Failure during
`CO_EXCEPTION` propagates after mandatory cleanup without recursively reporting
the new failure through that hook.

## Cancellation and cleanup

`CANCELLABLE` adds `TRY_CANCEL(@CONTROLLER MUT& C): BOOL` to the frame interface.
An invocation accepts one cooperative cancellation attempt. Delivery raises
`COROUTINE_CANCELLED` at an eligible await, yield, or ordinary coroutine call.
Catching that exception does not permit a second injection during the invocation.

An awaiter's `TRY_CANCEL` returns `TRUE` only after the operation is quiescent and
its notification has been exclusively claimed or retired. A failed claim retains
the pending attempt and registration. Frame cancellation does not resume an
unready operation. Construction, mandatory cleanup, and return/error delivery are
shielded; suspended shielded work requires its authorized readiness notification.

`CANCELLABLE(INSTANT)` permits forced exit through mandatory cleanup while skipping
body catch handlers. The selected cleanup must be synchronous. Coroutine
destructor preference still applies when an ordinary destructor alternative also
exists. Successful instant cancellation calls `CO_CANCELLED` after cleanup.

Coroutine lifecycle declarations use `.CONSTRUCTOR COROUTINE(...)` and
`.DESTRUCTOR COROUTINE()`, both with `VOID` completion. Coroutine contexts prefer
coroutine lifecycle declarations. Ordinary contexts select ordinary declarations.
Coroutine constructors are statically selected, including on polymorphic types.
They cannot declare virtual dispatch or override behavior. Ordinary `POLYMORPHIC`
types use `CONSTRUCTOR`. Separate `FULLOBJECT_CONSTRUCTOR` and
`SUBOBJECT_CONSTRUCTOR` declarations require `VIRTUAL_POLYMORPHIC`.
A coroutine destructor makes a type async-only unless a valid ordinary destructor
is available. `TYPE_IS_ASYNC_ONLY(T)` exposes that capability. Owning fields,
bases, arrays, and active alternatives propagate lifecycle requirements.

Scope exit, temporary destruction, rollback, replacement, `DELETE`, and
`DESTROY AT` use the shared lifetime planner. Partial construction retains its
initialized prefix. Object storage survives suspended cleanup, and deallocation
follows destruction. `DEFER` actions remain synchronous. Exception ownership and
cleanup progress persist in the frame; native unwinding does not remain active
across a driver return.

Each active cleanup context owns the exception being propagated and a separate
empty capture slot for a new failure. Nested cleanup preserves the parent's
exception ownership. A second exception during exceptional cleanup enters a
termination boundary before capture, preserving the ordinary double-exception
rule. Retained exception handlers detach before a driver returns and reattach
relative to the current caller when execution resumes.

## VMIR representation

The logical routine contains ordinary typed hook calls and three coroutine
operations:

| Operation | Contract |
| --- | --- |
| `co_save` | Prepares the continuation and required persistent state. |
| `co_suspend` | Terminates the activation while retaining coroutine-owned lifetimes. |
| `co_complete` | Completes required cleanup/reporting, publishes completion, and returns or propagates. |

Cleanup expansion makes coroutine destruction and rollback explicit suspendable
control flow. Exception-capture continuations acquire ownership before entering
such cleanup. Frame analysis runs on the logical CFG before generated driver
dispatch edges. Lowering produces ordinary VMIR frame accesses, calls, branches,
and returns before dependency collection and backend execution.

`co_save` does not require copying stable objects or issuing a hardware fence.
Stable objects can be constructed directly in frame storage. Optimization can
eliminate physical storage while preserving the observable size and alignment of
the nominal frame type.

## Frame construction

The baseline computes strongly connected components, condenses them into a DAG,
and assigns deterministic topological ranks. Definitions, uses, construction,
cleanup, and represented borrow obligations contribute conservative inclusive
rank intervals. An object is activation-local when its complete required interval
contains no possible suspension. Equal ranks overlap. Unknown ownership or alias
evidence widens the interval, potentially to the entire invocation.

Persistent objects use an interval allocator with size/alignment classes. The
analysis includes initialization-to-first-resume, ordinary jumps, logical resume
edges, exceptions, cancellation, and cleanup. Completed-frame reuse is a separate
invocation. Valid `GOTO` statements and irreducible cycles retain these rules.

Optional borrow and placement refinements share a deterministic work allowance
proportional to `N * ceil(log2(N + 1))`, together with a near-linear memory limit.
Budget exhaustion leaves unfinished proofs unknown and retains a valid
conservative result. The selected layout is the smaller completed valid layout.
Dense placement is skipped when the baseline already reaches the minimum extent
required by the header and each individual persistent object.
The budget is independent of wall-clock time, optimization level, and CPU
stepping. Stable logical identifiers determine traversal and placement ties.

Lifetime snapshots share an immutable radix-tree representation, and serialization
preserves the snapshot DAG. Distinct logical objects retain distinct ownership
identities when their physical storage overlaps. The constexpr interpreter uses
target byte offsets for pointer identity while retaining separate lifetime and
access checks.

Cleanup expansion uses a fixed work allowance derived from the typed routine
before cleanup generation. The allowance accounts for generated locals and
instructions. Exhaustion discards the optional expansion and regenerates cleanup
edges that reference shared source and destination lifetime snapshots. A resumable cursor
selects statically typed cleanup actions from a shared program. The frame stores
cursor progress, lifetime retirement flags, saved addresses, and nested exception
contexts. The dispatcher requires no heap allocation. Generated action bodies and
continuation destinations are shared across cleanup edges.

Suspendable action bodies are shared by selected destructor, object type, and
construction or subobject phase. Each invocation supplies its object address,
retirement identity, and continuation. A dependency graph of owned cleanup
operations bounds nested context storage and diagnoses recursive mandatory
cleanup. Budget exhaustion completes the current action before abandoning the
unpublished expansion. Immutable source lifetime snapshots remain shared across
the two attempts.

## Implementation locations

- [Coroutine syntax and controller lowering](../quxlang/include/quxlang/co_vmir_generator2.hpp).
- [Initialization and rollback](../quxlang/include/quxlang/co_coroutine_initialize.hpp).
- [Suspendable cleanup](../quxlang/include/quxlang/co_coroutine_cleanup.hpp).
- [Shared cleanup dispatch](../quxlang/include/quxlang/co_coroutine_cleanup_dispatch.hpp).
- [Cleanup program and cursor](../quxlang/include/quxlang/vmir2/lifetime_cleanup_program.hpp).
- [Frame query and nominal layout](../quxlang/sources/queries/coroutine_frame.cpp).
- [General-CFG analysis and interval allocation](../quxlang/sources/vmir2/coroutine_frame_analysis.cpp).
- [Driver lowering](../quxlang/sources/vmir2/coroutine_lowering.cpp).
- [Caller-owned and owning library adapters](../quxlang/tests/testdata/testbundle/modules/std/sources/coroutine.qxs).
- [Frame contract tests](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_coroutine_frame.qxs).
- [Awaiter protocol tests](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_coroutine_protocol.qxs).
- [Asynchronous lifecycle tests](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_coroutine_async_cleanup.qxs).
