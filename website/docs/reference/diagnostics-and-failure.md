# Failure Statements

Quxlang provides policy-controlled assertions, unconditional test checks,
panics, compilation errors, and configurable unimplemented-path markers.

For recoverable failures, use [Exception Handling](exceptions.md). `CATCH`
does not intercept failed assertions or `PANIC`.

## `ASSERT`

```quxlang
ASSERT(condition);
ASSERT(index < count, "index is outside the sequence");
```

The grammar is:

```text
ASSERT ( condition [ , string-literal ] ) ;
```

The condition must be usable as `BOOL`. The optional diagnostic tag must be a
string literal; it is not an arbitrary runtime expression. The compiler also
records the source text of the condition and its source location.

During compile-time execution, a false assertion raises a constexpr runtime
failure. In native runtime code, a false assertion calls
`MODULE(RUNTIME)::ASSERT_FAIL` with the condition text, file identifier, line,
column, and optional tag. A true assertion continues normally.

`ASSERT` evaluates its condition only when `ASSERT_ENABLED` is enabled. When
disabled, the entire assertion, including condition side effects, is omitted
from the lowered path. Constant evaluation enables the policy. See
[Compilation Policies](compilation-policies.md) for output defaults.

## `TEST_ASSERT` and `TEST_EXPECT`

Both statements accept the same condition and optional string-literal tag as
`ASSERT`, and always evaluate their condition once regardless of assertion
policy:

```quxlang
TEST_ASSERT(actual == expected, "result differs");
TEST_EXPECT(actual == expected, "result differs");
```

`TEST_ASSERT` uses the assertion failure path. `TEST_EXPECT` throws `TEST_FAILED`
on failure, allowing normal exception cleanup and a matching `CATCH` handler.
It does not continue executing the failed path unless a handler catches it.

## `PANIC`

```quxlang
PANIC;
PANIC "unrecoverable parser state";
```

`PANIC` accepts an optional string-literal message and ends the reached control
flow path. No statement after it in the same path is reachable.

At compile time, reaching `PANIC` raises a constexpr runtime failure. Native
lowering calls `MODULE(RUNTIME)::PANIC` with the selected message and source
location. When no message is supplied, the compiler uses a default panic
message.

## `COMPILATION_ERROR`

```quxlang
COMPILATION_ERROR;
COMPILATION_ERROR "this instantiation is unsupported";
COMPILATION_ERROR ON_LOWER "this path is unavailable in this execution mode";
```

The message is optional but, when present, must be a string literal.

Without `ON_LOWER`, generating the selected statement immediately raises a
semantic compilation error. This form is useful in a `STATIC_IF` branch or
template path that must never be selected:

```quxlang
STATIC_IF(BITS(I32) != 32)
{
  COMPILATION_ERROR "this implementation requires 32-bit I32";
}
```

`ON_LOWER` emits a lowering error into the generated path instead. The error is
reported if that path remains lowering-reachable. This distinction matters
because an ordinary `IF` generates both branches, even if its condition is
statically written as `FALSE`.

```quxlang
::native_only FUNCTION(): I32
{
  RUNTIME CONSTEXPR
  {
    COMPILATION_ERROR ON_LOWER
      "native_only cannot run during compile-time evaluation";
  }
  RETURN 42;
}
```

`STATIC_IF` and `RUNTIME NATIVE` can discard an unselected path before it
becomes a dependency. See [Compile-Time Evaluation](compile-time-evaluation.md)
and [Runtime Selection](runtime-selection.md).

## `UNIMPLEMENTED`

```quxlang
UNIMPLEMENTED;
```

`UNIMPLEMENTED` takes no message and terminates the reached path. The target
setting `unimplemented_compiles` defaults to `true`. Setting it to `false`
rejects the statement during code generation.

When compilation is permitted, `UNIMPLEMENTED_PANICS` selects its behavior:
when enabled, reaching it panics; when disabled, a lowering-reachable statement
is a compilation error. The policy defaults to disabled for `Release` and
`ReleaseDbgSym` and enabled for other build types. Constant evaluation enables
it and fails if execution reaches the statement.

See [Compilation Policies](compilation-policies.md) and
[The `qxcbuild.yml` File](qxcbuild-file.md).

## Expected failures in static tests

Failure phase is part of the testing contract:

```quxlang
::panic_is_expected STATIC_TEST EXPECT_FAIL
{
  PANIC "expected compile-time execution failure";
}

::rejection_is_expected STATIC_TEST EXPECT_COMPILATION_FAILURE
{
  COMPILATION_ERROR "expected semantic rejection";
}
```

`EXPECT_FAIL` accepts a constexpr runtime failure, such as a failed assertion
or reached panic. `EXPECT_COMPILATION_FAILURE` accepts a compilation failure.
Only `STATIC_TEST` permits these modifiers, and a test that unexpectedly
succeeds is itself diagnosed. See [Tests](tests.md) for test declaration rules.
