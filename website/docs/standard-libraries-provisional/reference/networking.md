# Networking

The `net` module provides TCP connections, TCP listeners, and UDP sessions on Linux,
macOS, and Windows. Socket interfaces expose IP endpoints and portable error
categories. Native handles and native error numbers are implementation details.

The `syscall` module declares native socket procedures and ABI structures.
Platform procedures belong to `syscall::linux`, `syscall::macos`, and
`syscall::windows`. The `net` module implements resource ownership, endpoint
conversion, and portable error translation.

## Resource types

| Reference | Automatic owner | Operations |
| --- | --- | --- |
| `tcp_connection_ref` | `tcp_connection` | `bind`, `connect`, `read`, `write`, `shutdown` |
| `tcp_listener_ref` | `tcp_listener` | `bind`, `listen`, `accept` |
| `udp_session_ref` | `udp_session` | `bind`, `send`, `receive` |

All resource types provide `local_endpoint` and `close`. References provide
`is_valid`, which reflects the stored handle. Reference copies identify the same
socket and do not extend its lifetime. Closing one reference ends the lifetime of
the socket for every copy. Using a copy after closure violates the lifetime contract.

Automatic owners are move-only. Default construction creates a closed owner.
Construction with `@address` creates a socket for the address's IP version.
Construction with `@reference` adopts the exclusive closing obligation.
`reference()` returns a reference, `release()` transfers the closing obligation
and empties the owner, and `is_open()` reports ownership. Moving an owner empties
the source. Destruction closes an owned socket once. `close()` releases ownership
before calling the operating system and returns whether the close succeeded.

The owner and socket remain alive until every pending operation completes.
The owner's storage stays at a stable address while a coroutine operation invoked
through that owner is pending. Moves and releases occur after those coroutine
operations complete. Operations in the same direction require caller serialization. One read and one
write may run concurrently on a connection. A reference has no per-resource
allocation or operation bookkeeping.

## Addresses

`ipv4_address::data` contains four network-order `U8` bytes.
`ipv6_address::data` contains sixteen network-order `U8` bytes.
`ip_address` is an inline variant of those address types, with IPv4 as its default
alternative.

`endpoint` contains `address`, a host-order `U16` `port`, and a `U32` `scope_id`.
The scope identifies an IPv6 interface. Port zero requests an ephemeral local port
when binding. `local_endpoint()` returns the selected port after binding.

```quxlang
VAR address net::ipv4_address;
address.data[0] := 127;
address.data[3] := 1;
VAR local net::endpoint;
local.address := net::ip_address(@OTHER address);
VAR listener net::tcp_listener :(@address local.address);
listener.bind(@endpoint local);
listener.listen(@backlog 128);
local := listener.local_endpoint();
```

## Transfers

`connect(@endpoint)` establishes a TCP connection. `accept()` returns an owning
`tcp_connection`. A listener is bound before `listen(@backlog)` starts listening.
The backlog defaults to 128 and must be positive.

`read(@buffer)` and `write(@buffer)` transfer one TCP prefix and return its `SZ`
byte count. Buffers use `io::buffer_ref`. A nonempty read that returns zero reports
orderly end of stream. Empty reads and writes return zero without a native call.
TCP operations may transfer fewer bytes than requested. A native transfer is
limited to 2,147,483,647 bytes.

`shutdown(@direction)` disables receiving, sending, or both through
`shutdown_direction::receive`, `shutdown_direction::send`, or
`shutdown_direction::both`. Shutdown preserves the closing obligation.

`send(@buffer, @destination)` sends one UDP message and returns its byte count.
A message larger than 2,147,483,647 bytes fails with `error::message_too_large`.
`receive(@buffer)` consumes one message and returns `datagram_result`, containing
`size`, `source`, and `truncated`. A truncated message reports the copied prefix
and discards the rest of that message. An empty datagram succeeds with size zero.
An empty receive buffer consumes the next datagram and reports truncation when
that datagram contains bytes.

## Asynchronous execution

Each of `connect`, `accept`, `read`, `write`, `send`, and `receive` has an explicit
`async_` form with an additional `@runtime MUT& io::async_runtime` argument. The
result is a single-consumer `std::future` whose value type matches the synchronous
operation's result. Connection establishment returns `std::future#VOID`.

The same operation names also select coroutine overloads under
`io::async_controller`. `ON(runtime) RUN` selects that controller for an invocation
and its nested coroutine calls. Socket operations suspend their caller until the
future completes. Native errors propagate through the future or coroutine result.

```quxlang
::read_connection COROUTINE(
  @connection net::tcp_connection_ref,
  @buffer io::buffer_ref
): SZ
{
  RETURN connection.read(@buffer buffer);
}

// The connection and buffer storage outlive pending.get().
VAR pending := ON(runtime) RUN read_connection(
  @connection connection.reference(), @buffer buffer);
VAR transferred SZ := pending.get();
```

Socket operations submit native io_uring requests on Linux. The epoll backend,
selected when `io::linux_use_io_uring` is false, and macOS kqueue use nonblocking
transport with native readiness notifications.
Windows operations use overlapped Winsock calls delivered through the runtime's
I/O completion port, including `ConnectEx` and `AcceptEx`. Pending sockets consume
no file-worker threads. The `file_workers` argument controls synchronous file
transfers. Synchronous socket calls wait for readiness on their calling thread.

`drain()` waits for scheduled coroutines, file-worker operations, native requests,
and completion cleanup. External submissions stop before draining or destroying
a runtime. Socket operations provide no deadline or active cancellation interface.
Pending future awaits retain their coroutine frames through completion. Windows
sockets bind to the first runtime that submits an asynchronous operation; subsequent
operations require that same runtime.

`io::async_runtime::submit(@operation)` owns an operation at a stable address and
returns `std::future#(operation_type::value_type)`. The operation supplies these
members:

- `value_type` identifies the completed value.
- `start(@submission CONST& io::async_submission_ref): BOOL` starts native I/O.
- `complete(@submission CONST& io::async_submission_ref, @size SZ, @error U32): BOOL`
  processes a native notification. The error is the platform's positive error code,
  or zero for success. io_uring supplies the nonnegative native completion result
  as size; kqueue and epoll readiness notifications supply zero.
- `result(): value_type` produces the final value, including `VOID`.

Both `start` and `complete` return `TRUE` for final completion. A `FALSE` return
requires exactly one pending native notification. Exceptions complete the future
with an error and require no pending notification. Completion and submission are
serialized for each operation. Linux io_uring operations submit a native entry
through `submission.submit(@entry)`. POSIX operations register readiness through
`submission.wait(@descriptor, @writing)`. Windows operations associate their native
handle through `submission.associate(@handle)` and submit overlapped I/O using
`submission.overlapped()`. Operation storage and native request state remain alive
until the final callback returns.

## Failures

Native socket failures throw `net::error`, whose enumerators are `invalid_argument`,
`closed`, `permission_denied`, `address_in_use`, `address_unavailable`,
`connection_refused`, `connection_reset`, `connection_aborted`,
`network_unreachable`, `timed_out`, `would_block`, `interrupted`, `resource_limit`,
`message_too_large`, `unsupported`, and `system_failure`. Native error numbers are
translated at the platform boundary. Interrupted TCP and UDP transfer calls retry
before publishing their result. Explicit close operations return their outcome as
`BOOL`. Native queue registration failures propagate as
`io::async_error::submission_failed`.

Socket creation establishes the platform's descriptor inheritance policy. Linux
and Windows create sockets with inheritance disabled. Darwin configures that policy
before publishing the resource. TCP writes suppress process-level broken-pipe
signals on POSIX platforms. Each Windows socket retains a Winsock initialization
reference until closure.
