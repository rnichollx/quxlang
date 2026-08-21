# Concurrency and Per-Thread Storage

The current language surface provides per-thread globals and atomic types;
higher-level thread, mutex, condition-variable, and semaphore ownership lives in
the `std` module.

## Per-thread objects

```quxlang
::thread_count PER_THREAD VAR I32 := 17;
```

Every thread receives an independent instance. Nontrivial instances are
initialized for the thread and destroyed when that thread's Quxlang lifetime is
drained. Destructors run in reverse order of completed per-thread
initialization.

`PER_THREAD VAR` is valid only as a global declaration. References to one
thread's instance must not be used as if they referred to another thread's
instance.

## Atomic objects

`ATOMIC#T` is the compiler-provided atomic type for a supported payload type:

```quxlang
::publish FUNCTION(@ARG:value I32): I32
{
  VAR state ATOMIC#I32;
  state.STORE#ATOMIC_RELEASE(value);
  RETURN state.LOAD#ATOMIC_ACQUIRE();
}
```

Atomic operations select their memory order as a template argument. Integer
atomics provide read-modify-write and compare-exchange operations. Their full
operation and memory-order contracts are documented on
[Atomic objects](atomics.md).

Use higher-level `std::thread`, `std::mutex`, `std::condition_variable`, and
`std::semaphore` where their platform predicates make them available. Their
ownership and cleanup remain ordinary Quxlang constructor/destructor behavior.
