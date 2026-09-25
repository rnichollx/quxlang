# Overview of Failure Statements

Quxlang provides explicit assertions, runtime termination, compile-time
rejection, and unimplemented-path markers.

For recoverable failures, use [Exception Handling](exceptions.md). `CATCH`
does not intercept failed assertions or `PANIC`.

## Assertions

```quxlang
ASSERT(condition);
ASSERT(condition, "condition was false");
```

When `ASSERT_ENABLED` is enabled, an assertion failure terminates the current execution mode. In a `STATIC_TEST`
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

`unimplemented_compiles: false` rejects this statement during code generation.
With the default `true`, `UNIMPLEMENTED_PANICS` selects a panic when enabled or
a compilation error for a lowering-reachable path when disabled. Release builds
disable this policy by default.

Expected-failure test declarations are documented on [Tests](tests.md), and
the target setting is listed under
[The `qxcbuild.yml` File](../reference/qxcbuild-file.md).

## Assertions in tests

Use `TEST_ASSERT(condition)` for an assertion that always runs, or
`TEST_EXPECT(condition)` to throw `TEST_FAILED` on failure. Both accept an
optional diagnostic string. Ordinary `ASSERT` can omit its condition entirely
under the output's [compilation policies](compilation-policies.md).

## Reference

See the [Failure Statements Reference](../reference/diagnostics-and-failure.md) for the complete
language rules, constraints, and technical edge cases.
