# Thread-Local Variables

`VAR PER_THREAD` declares a global variable with one independent object per
thread. It is Quxlang's language-level thread-local storage declaration.

## Syntax and placement

The complete declaration form is:

```quxlang
::name VAR PER_THREAD Type initializer;
```

For example:

```quxlang
::current_depth VAR PER_THREAD U32 := 0;
::thread_cache VAR PER_THREAD cache;
```

The declaration must be global. `VAR PER_THREAD` is not valid for a local
variable or a structure member, and `PER_THREAD` follows `VAR`.

The type and initializer follow the ordinary namespace-scope
[variable](variables.md) rules. Omitting the initializer invokes the
no-argument constructor. The `:=`, `:(...)`, and `:[...]` forms select the same
constructor paths as an ordinary global variable.

## Per-thread identity

Evaluating the variable's name accesses the instance belonging to the current
thread:

```quxlang
::thread_total VAR PER_THREAD I32 := 0;

::add_for_current_thread FUNCTION(@amount I32): I32
{
  thread_total := thread_total + amount;
  RETURN thread_total;
}
```

Changing `thread_total` in one thread does not change its value in any other
thread. A default-initialized integer is zero independently in each thread; an
explicit initializer such as `:= 17` supplies the initial value independently
for each thread.

`VAR PER_THREAD` does not make operations atomic. It avoids sharing the
declared object because ordinary name lookup selects separate storage. If code
deliberately publishes an address to one thread's instance, that address still
denotes the original instance; it does not retarget when used by another
thread. The program must preserve the original instance's lifetime and provide
any synchronization required for cross-thread access.

## Initialization

Each thread has a separate initialization state for every `PER_THREAD`
variable. Trivial variables may be supplied directly by the target's TLS image.
When construction requires guarded initialization, the first access in a
thread constructs that thread's instance.

Initialization is complete only after the constructor succeeds. Recursive
initialization of the same per-thread object is invalid and causes a runtime
failure. Initialization is also forbidden once that thread has begun draining
its thread-local objects.

These rules apply per declaration and per thread. Accessing one per-thread
variable while constructing another is permitted unless it creates an
initialization cycle.

## Destruction

A successfully initialized per-thread object with a nontrivial destructor is
registered for destruction in that thread. At thread-local teardown:

- only instances whose initialization completed are destroyed;
- destruction occurs in reverse order of completed initialization;
- an instance is destroyed at most once;
- accessing an instance that has already been destroyed is a runtime failure;
- starting a new guarded per-thread initialization while teardown is active is
  a runtime failure.

The reverse order is based on the order in which the thread actually completed
initialization, not source declaration order. This matters because guarded
instances may first be accessed in different orders on different threads.

Thread-local teardown requires the runtime to observe thread exit. The runtime
module provides this integration for Quxlang-created threads and for the
supported hosted threading environments described by the
[Runtime Module Contracts](runtime-module-contracts.md).

## Relationship to other features

`VAR PER_THREAD` chooses storage identity; it does not create a thread, mutex,
or communication channel. Thread creation and synchronization types are
library facilities. Atomic memory-ordering rules are documented separately in
[Atomics](atomics.md).

`CONSTEXPR_OK` may follow `VAR PER_THREAD` to permit access during constant
evaluation. `STATIC` and `STATIC_VAR` cannot be combined with `PER_THREAD`. An ordinary namespace-scope `VAR` instead denotes one
program-wide object shared by all threads.
