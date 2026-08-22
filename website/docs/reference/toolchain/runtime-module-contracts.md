# Runtime Module Contracts

Every target maps one source module to the logical `RUNTIME` module. That
module supplies reserved declarations used to implement language operations;
application modules cannot redeclare them. Application source normally uses
`ASSERT`, `PANIC`, `NEW`, `DELETE`, global objects, and `PER_THREAD VAR` while
the compiler connects those features to the contracts on this page.

## Diagnostics

Runtime assertion and panic handling use fixed named-parameter signatures:

```quxlang
::ASSERT_FAIL FUNCTION(
  @expr STRING_CONSTANT,
  @file SZ,
  @line SZ,
  @column SZ,
  @tag CONST->STRING_CONSTANT
)
{
  UNIMPLEMENTED;
}

::PANIC FUNCTION(
  @message STRING_CONSTANT,
  @file SZ,
  @line SZ,
  @column SZ
)
{
  UNIMPLEMENTED;
}
```

`ASSERT_FAIL` receives the source expression text, source-file index, line,
column, and optional assertion tag. `PANIC` receives the message and source
position. These functions must not return to the failed source path.

On Windows native targets, `::CHECK_STACK` is an architecture-specific
`ASM_PROCEDURE` used for stack probing. A Windows LLVM output that needs the
probe requires that declaration in `RUNTIME`.

## Default allocator

`NEW` and `DELETE` resolve typed single-object storage through
`::DEFAULT_ALLOCATOR`:

```quxlang
::DEFAULT_ALLOCATOR STRUCT
{
  ::allocate TEMPLATE(@T TYPE AUTO(t))
    FUNCTION(): ->TYPED_STORAGE(t)
  {
    UNIMPLEMENTED;
  }

  ::dealloc TEMPLATE(@T TYPE AUTO(t))
    FUNCTION(@ptr ->TYPED_STORAGE(t))
  {
    UNIMPLEMENTED;
  }
}
```

The allocator returns storage rather than a live `T`; the language operation
performs construction after allocation and destruction before deallocation. A
runtime can select among compile-time, native, and managed implementations with
`RUNTIME CONSTEXPR`, `RUNTIME NATIVE`, and target predicates. Native allocators
provide native storage, while the JVM path uses managed object-storage
builtins.

A runtime library may also expose `allocate_multiple` and `dealloc_multiple`
members returning and accepting `=>>TYPED_STORAGE(t)`, plus explicit
`@size`/`@align` overloads returning `ALIGNED_STORAGE(size, align)`. Those are
ordinary runtime-library APIs; the typed `allocate#T` and `dealloc#T` members
above are the reserved single-object language integration points.

## Global initialization guards

The runtime module alone may name `INITGUARD`. It supplies three operations for
thread-safe initialization of nontrivial global objects:

```quxlang
::INITGUARD_TRY_ACQUIRE FUNCTION(@guard MUT& INITGUARD): BOOL
{
  UNIMPLEMENTED;
}

::INITGUARD_COMPLETE FUNCTION(@guard MUT& INITGUARD)
{
  UNIMPLEMENTED;
}

::INITGUARD_ABORT FUNCTION(@guard MUT& INITGUARD)
{
  UNIMPLEMENTED;
}
```

`INITGUARD_TRY_ACQUIRE` returns `TRUE` to the caller responsible for
initialization and `FALSE` once the object is already initialized. Contending
callers wait for that decision. The successful path completes the guard after
construction; a failed or unwinding path aborts it so a later attempt can
retry.

These declarations are runtime hooks. Application source declares the global
object and does not manipulate its guard directly.

## Per-thread initialization and destruction

Nontrivial `PER_THREAD VAR` objects use the corresponding per-thread contract:

```quxlang
::thread_destructor_node STRUCT
{
  .next VAR MUT->thread_destructor_node;
  .deinitializer VAR CONST->PROCEDURE();
  .guard VAR MUT->INITGUARD;
}

::THREAD_INITGUARD_TRY_ACQUIRE
  FUNCTION(@guard MUT& INITGUARD): BOOL
{
  UNIMPLEMENTED;
}

::THREAD_DESTRUCTOR_REGISTER FUNCTION(
  @node MUT& thread_destructor_node,
  @guard MUT& INITGUARD,
  @deinitializer CONST->PROCEDURE()
)
{
  UNIMPLEMENTED;
}
```

The per-thread acquire operation distinguishes uninitialized, initialized,
recursively initializing, and destroyed states. After successful construction,
registration adds the object's deinitializer to the current thread. The
runtime drains registered destructors in reverse completion order and marks
their guards destroyed.

The current runtime also reserves `::THREAD_RUNTIME_START` and
`::THREAD_FINISH` as the entry and exit boundary for a Quxlang-managed thread.
They expose operations that establish thread identity and drain per-thread
lifetime before native thread teardown; a platform integration may call that
boundary when it creates or finishes a managed thread.

```quxlang
::THREAD_RUNTIME_START FUNCTION(@identity MUT->thread_identity)
{
  UNIMPLEMENTED;
}

::THREAD_FINISH FUNCTION()
{
  UNIMPLEMENTED;
}
```

## Startup and test entrypoints

`::PROGRAM_START`, `::POST_DETECT`, and `::UNIT_TEST_MAIN` are also reserved to
the runtime module. Their signatures, stepping tables, and dispatch order are
documented on [Program startup and runtime hooks](program-startup-and-runtime-hooks.md).
Runtime CPU detector declarations use the reserved `::DETECT_<capability>`
names described on
[CPU capabilities and steppings](cpu-capabilities-and-steppings.md).

See [`NEW` and `DELETE`](../new-and-delete.md),
[Thread-Local Variables](../thread-local-variables.md),
and [Diagnostics and explicit failure](../diagnostics-and-failure.md).
