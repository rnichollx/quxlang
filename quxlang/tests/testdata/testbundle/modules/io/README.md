# Standard I/O module

The `io` module provides borrowed native descriptors, file ownership, and
asynchronous file operations. A target that uses this module declares
`io: {source: io}` in its module map. Source files import it with `IMPORT io;`.

`io::io_descriptor_ref` stores a native POSIX descriptor or Windows handle.
`native_handle()` exposes the stored value to system interfaces, and `is_valid()`
checks the platform's invalid values. Copies borrow the same resource.

`io::file_ref` is a resource reference object. Its `descriptor` field contains an
`io_descriptor_ref`. Its `read`, `write`, and `close` operations act directly on
the resource. Destruction performs no resource operation. Closing a reference
invalidates every reference to that native resource; the reference values do not
track this change.

`io::file` is a move-only automatic resource management object. Construction from
`@native_ref` transfers responsibility for closing the file. `reference()` borrows
the resource, and `release()` transfers responsibility back to the caller. Moving
empties the source. Destruction closes an owned file. Explicit `close()` releases
ownership before attempting the native close and reports its success.

A transfer accepts `io::buffer_ref(@data bytes, @size count)` and writes an
`io::transfer_result` with `size` and `succeeded` fields. Successful transfers may
be partial. A successful zero-byte read indicates end of file. The buffer's
storage outlives every operation that uses it.

## Coroutine execution

`io::async_runtime` owns a `std::thread_pool_cort_runtime` for coroutine execution,
a platform completion queue, completion workers, and workers for cursor-based
file operations. `ON(runtime) RUN operation(...)` returns a standard future.
Nested coroutine calls and `GO` operations retain the runtime's I/O controller.
The runtime drains scheduled coroutine frames and their cleanup before stopping
completion workers. Submission from other threads must stop before destruction.

```quxlang
::read_block COROUTINE(@source io::file_ref, @buffer io::buffer_ref): io::transfer_result
{
  VAR result io::transfer_result;
  source.read(@buffer buffer, @offset 0, @result result);
  RETURN result;
}
```

Controller-selected `read` and `write` overloads suspend until completion.
Overloads with `@offset U64` submit positioned native transfers. Overloads without
an offset perform cursor-based transfers on file workers; the operating system
manages the shared file position. Ordinary synchronous calls use the native
cursor-based operations directly. A file owner, its references, and buffer storage
remain valid through completion. Concurrent operations require independently
synchronized buffers. Windows positioned operations require handles opened with
`FILE_FLAG_OVERLAPPED`; a handle belongs to one completion port.

The runtime constructor accepts positive `@workers`, `@completion_workers`,
`@file_workers`, and `@entries` counts. Their defaults are 4, 1, 2, and 256.
The completion worker count cannot exceed the entry count. Queue creation failure
throws `io::async_error::initialization_failed`. Submission and native transfer
failures produce an unsuccessful transfer result. Requests retain their state
until completion, including when a future consumer releases its ownership.

## Platform selection

macOS uses `io::kqueue_async_runtime`, which receives POSIX AIO completions.
Windows uses `io::overlapped_async_runtime`, which receives I/O completion-port
packets. Linux selects its backend with the Boolean `io::linux_use_io_uring`
`OPTION`, whose default value is `TRUE`.

```yaml
io:
  source: io
  options:
    linux_use_io_uring: false
```

The default Linux backend is `io::io_uring_async_runtime`. Setting the option to
`FALSE` selects `io::epoll_async_runtime`. The epoll backend executes positioned
file transfers on I/O workers and delivers their completion records through an
eventfd registered with epoll. Regular files do not provide epoll readiness
notifications. The selected platform runtime exposes the `async_context` queue
API, including callback submissions and `wait_one()`. `io::thread_pool` owns
completion consumers for that API; `io::async_runtime` manages these consumers
automatically. Completion callbacks complete without throwing.
