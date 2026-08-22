# Overview of Failure Statements

Quxlang provides explicit assertions, runtime termination, compile-time
rejection, and unimplemented-path markers.

## Assertions

```quxlang
ASSERT(condition);
ASSERT(condition, "condition was false");
```

An assertion failure terminates the current execution mode. In a `STATIC_TEST`
it is a static execution failure; in runtime code it follows the runtime
assertion path.

## Panic

```quxlang
PANIC "unrecoverable state";
```

`PANIC` is a terminal statement. Its message is optional.

## Compilation errors

```quxlang
COMPILATION_ERROR "this source configuration is unsupported";
COMPILATION_ERROR ON_LOWER "this path cannot be lowered";
```

`COMPILATION_ERROR` rejects a selected source path. `ON_LOWER` delays the error
until the statement is lowering-reachable, which lets an explicit runtime-mode
or target selection keep an unsupported implementation out of unrelated paths.

## Unimplemented paths

```quxlang
UNIMPLEMENTED;
```

`UNIMPLEMENTED` marks a terminal path whose behavior follows the compiler's
configured unimplemented-statement mode. The target setting
`unimplemented_mode: trap` emits a runtime trap for a reached path;
`unimplemented_mode: error` rejects a path reached during code generation. It
should not be used as a silent fallback for a valid result.

Expected-failure test declarations are documented on [Tests](tests.md), and
the target setting is listed under
[The `qxcbuild.yml` File](../../reference/toolchain/qxcbuild-file.md).

## Complete technical rules

See the [Failure Statements Reference](../../reference/diagnostics-and-failure.md) for the complete
language rules, constraints, and technical edge cases.
