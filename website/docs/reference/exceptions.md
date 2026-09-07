# Exception Handling

Quxlang exceptions propagate owned values across calls and scopes. A handler
binds a reference to the exception object. Exceptions are unchecked: ordinary
functions do not declare a list of exception types they may throw.

For introductory examples, see the [Overview](../overview/exceptions.md).

## Statements and handler order

```text
THROW expression;
THROW UNWIND_OUT_OF_MEMORY;
RETHROW;

TRY { statements }
CATCH name CONST& Type { statements }
CATCH name MUT& Type { statements }
CATCH UNWIND_OUT_OF_MEMORY { statements }
CATCH DEFAULT { statements }
```

A `TRY` requires at least one catch. Typed and sentinel catches may appear in
any source order. `CATCH DEFAULT` is optional, may occur only once, and must be
last. Every body is a braced block.

The first matching catch runs. A normal exit from it continues after the
entire `TRY` statement. An unmatched exception propagates to an enclosing
protected scope or caller. An exception thrown from a handler is considered
by enclosing handlers; sibling catches of that same `TRY` do not handle it.

Control flow cannot jump into a protected `TRY` or catch region from outside
it. Exits must perform the required lifetime cleanup. `RETURN` cannot escape
a deferred action, including from a catch nested inside that action.

## Thrown objects

`THROW expression;` evaluates the expression and constructs a complete owned
runtime value in exception storage, using the applicable construction and
forwarding rules. The exception object has its own lifetime. No distinguished
user exception base class is required.

A bare literal type, `VOID`, or an uninitialized/destroyed storage slot is not
a throwable owned value. Give a literal a concrete type:

```quxlang
TRY { THROW I32(@OTHER 42); }
CATCH error CONST& I32 { ASSERT(error == 42); }
```

Failure while constructing a payload propagates that failure; already
constructed parts are cleaned up. Failure to allocate exception storage uses
the [allocation-failure sentinel](#allocation-failure-sentinel).

## Typed matching

A typed binding must be `CONST& T` or `MUT& T`. Value catches and `TEMP&` or
`WRITE&` catch bindings are rejected. A top-level pointer such as `CONST->T`
is not a catch reference.

Matching first checks the exact payload type. For a polymorphic payload, a
catch may also bind a unique subobject through a permitted dynamic cast.
Ambiguous repeated bases do not match. Numeric conversions, user conversion
constructors, and ordinary nonpolymorphic base conversions do not select a
handler.

```quxlang
::failure STRUCT POLYMORPHIC { .code VAR I32; }
::read_failure STRUCT POLYMORPHIC { .parent BASE failure; }
```

```quxlang
VAR caught BOOL := FALSE;
TRY
{
  VAR error read_failure;
  error.code := 17;
  THROW error;
}
CATCH error CONST& failure
{
  caught := error.code == 17;
}
ASSERT(caught);
```

`CONST&` prevents mutation through that binding. `MUT&` permits mutation of the
stored object; changes are visible through saved handles and subsequent
rethrows. A handler reference does not own the exception. Any reference kept
past the handler must be backed by a retained `EXCEPTION_PTR` for as long as
it is used.

## Rethrowing and active handlers

`RETHROW;` propagates the exception of the lexically enclosing catch in the
same callable. It preserves object identity and mutations. Outside such a
catch it is a compilation error. A nested callable does not inherit this
lexical permission.

`CURRENT_EXCEPTION()` instead observes the dynamically active handler on the
current thread. A function called by a handler sees that handler's exception.
A nested handler temporarily becomes current; leaving it restores the
previous handler.

An exception being unwound does not become current until its handler is
entered. Cleanup therefore sees the previously active handler, or an empty
handle if there was none. A destructor can catch another exception locally
without replacing the exception that is propagating through its caller.

## `EXCEPTION_PTR` API

`EXCEPTION_PTR` is the reserved owning exception-handle type implemented by
the runtime module. Application source uses its name directly.

| Operation | Result and contract |
| --- | --- |
| `VAR exception EXCEPTION_PTR;` | Constructs an empty handle |
| Copy construction or assignment | Shares ownership of the same exception |
| Move construction | Transfers ownership and leaves the source empty |
| `exception??` | `BOOL`; true for an owned exception, including the sentinel |
| `exception.IS_OUT_OF_MEMORY()` | `BOOL`; true only for the allocation-failure sentinel; false for an empty handle |
| `left == right` | `BOOL`; compares exception identity, with two empty handles equal |
| `CURRENT_EXCEPTION()` | `EXCEPTION_PTR`; retains the active handler's exception, or returns empty; `NOEXCEPT` |
| `THROW_EXCEPTION_PTR(@exception handle)` | Propagates the exception retained by a nonempty `CONST& EXCEPTION_PTR` |

Presence, sentinel, and equality queries are `NOEXCEPT`. The API exposes no
reference-count query. Final release destroys the payload and frees its
exception storage; the sentinel has a permanent runtime-owned reference.

`THROW_EXCEPTION_PTR` preserves the original payload and does not consume the
caller's handle. Passing an empty handle fails an assertion. `THROW handle;`
would construct an exception whose payload is the handle itself; use
`THROW_EXCEPTION_PTR` to propagate the exception it identifies.

Handle ownership is reference-counted with atomic operations. Active-handler
state is thread-local. Shared ownership does not synchronize mutations of the
payload or concurrent writes to the same handle variable.

## Allocation-failure sentinel

`UNWIND_OUT_OF_MEMORY` represents inability to allocate exception storage,
including storage needed for propagation. It has no ordinary object payload
and never matches a typed object catch. Both `CATCH UNWIND_OUT_OF_MEMORY` and
`CATCH DEFAULT` can handle it.

The runtime keeps the sentinel permanently alive. `THROW UNWIND_OUT_OF_MEMORY;`
raises it explicitly. Within its handler, `CURRENT_EXCEPTION()` is nonempty
and `IS_OUT_OF_MEMORY()` is true. It can be saved and rethrown like any other
exception handle.

Sentinel handling does not guarantee that arbitrary recovery code can
allocate. Exhaustion of the runtime's reserved propagation capacity terminates
execution. The sentinel is specific to exception handling; this contract does
not change every allocator failure into a catchable exception.

## Cleanup and `NOEXCEPT`

Propagation destroys live locals in the scopes it exits. Partially
constructed objects clean up their completed fields, bases, or array elements;
an incompletely constructed object does not receive a complete-object
destructor call. Cleanup does not destroy the exception object while a
handler or saved handle still owns it.

`DEFER expression;` or `DEFER { statements }` installs an action for scope
exit. Deferred actions run in reverse lifetime order, alongside ordinary
local destruction, on both normal and exceptional exits.

A callable marked `NOEXCEPT` must handle exceptions before they escape its
boundary. It may call potentially throwing functions and catch locally.
An escaping exception terminates execution before an outer caller's catch can
recover from it.

Destructors and deferred actions are implicitly `NOEXCEPT`. This also applies
to generated destruction of subobjects. A destructor may handle a throw in its
own body, but cannot recover from an exception escaping a subobject's
`NOEXCEPT` destructor. An exception escaping cleanup terminates even when no
other exception was already propagating.

Procedure types can include `NOEXCEPT`; see
[Procedure Pointers and Function Values](procedure-pointers-and-function-values.md).
An uncaught exception also terminates native execution. `ASSERT`, `PANIC`, and
reached `UNIMPLEMENTED` failures are not caught by language handlers; see
[Failure Statements](diagnostics-and-failure.md).

## Universal polymorphic view

`POLYMORPHIC_BASE` is a compiler built-in type. Every polymorphic object has
one canonical view through it. Converting pointers from different polymorphic
subobjects of the same complete object yields the same view, preserving
nullness and access qualification.

```quxlang
VAR object read_failure;
VAR view MUT->POLYMORPHIC_BASE := object.parent<-;
ASSERT((view->).DYNAMIC_TYPE() == TYPE_INDEX_OF(read_failure));
ASSERT((view AS DYNAMIC MUT->read_failure) == object<-);
```

`DYNAMIC_TYPE()` returns a `TYPE_INDEX` through a const receiver and is
`NOEXCEPT`. Runtime type reporting follows the active construction or
destruction phase, as described for
[`DYNAMIC_TYPE_OF`](type-queries-and-deduction.md#dynamic-type-identity).
`AS DYNAMIC` performs checked casts and `DELETE` through this view destroys
the complete allocated object using virtual destruction.

The view introduces no ordinary or virtual base subobjects. It cannot be
constructed as a standalone object or explicitly declared with `BASE`.
See [Inheritance](inheritance.md) for ordinary base-subobject rules.

## Constant evaluation and native support

Caught exceptions, saved handles, rethrows, and cleanup are supported during
constant evaluation, including `STATIC_TEST`. An uncaught exception or a
`NOEXCEPT` escape is a constexpr execution failure, not a recoverable compiler
diagnostic. Tests expecting that failure use `STATIC_TEST EXPECT_FAIL`;
invalid syntax or binding types use `EXPECT_COMPILATION_FAILURE`.

Native exception handling uses the Quxlang unwinder in `MODULE(RUNTIME)`.
Execution has been validated on macOS ARM64 and Linux x86, x86-64 (static and
glibc), ARM64, and s390x. Windows x64 cross-compiles; Windows execution remains
unverified. Exception handling on the Cortado/JVM backend is not implemented.

Native propagation currently covers frames in the Qxc-produced executable.
Foreign C++/SEH exception translation and unwinding through external shared
libraries are not implemented. Handle exceptions before crossing those
boundaries. The native runtime does not depend on libunwind or libgcc.
