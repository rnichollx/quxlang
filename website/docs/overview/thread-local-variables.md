# Overview of Thread-Local Variables

Use `PER_THREAD VAR` when every thread needs its own instance of a global
variable. Reading or changing the name affects only the current thread's
instance.

```quxlang
::current_depth PER_THREAD VAR U32 := 0;

::enter FUNCTION(): U32
{
  current_depth++;
  RETURN current_depth;
}

::leave FUNCTION()
{
  current_depth--;
}
```

If two threads call `enter`, each begins with its own `current_depth` initialized
to zero. Neither call changes the other thread's value.

## Initialization

Thread-local declarations use the same initializer forms as ordinary global
variables:

```quxlang
::thread_number PER_THREAD VAR I32 := 17;
::thread_buffer PER_THREAD VAR [64]BYTE;
```

Omitting the initializer invokes the type's no-argument constructor for each
thread. Nontrivial objects are initialized independently when that thread needs
them.

## Objects with destructors

A thread-local object with a destructor is destroyed during that thread's
teardown if its initialization completed. If several such objects were used,
they are destroyed in reverse order of completed initialization.

This makes thread-local objects useful for per-thread caches or state that owns
resources:

```quxlang
::active_session PER_THREAD VAR session_state;

::session_for_current_thread FUNCTION(): MUT& session_state
{
  RETURN active_session;
}
```

The returned reference denotes the current thread's instance. It should not be
treated as though it will select a different instance after being passed to
another thread.

## What it does not do

`PER_THREAD VAR` does not create threads and does not make operations atomic.
It changes which object a global name selects. Use an ordinary `VAR` for one
program-wide object, and use [Atomics](atomics.md) or library synchronization
when threads intentionally share an object.

## Reference

For placement restrictions, initialization cycles, teardown rules, and runtime
integration, see the
[Thread-Local Variables Reference](../reference/thread-local-variables.md).
