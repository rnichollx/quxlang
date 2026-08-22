# Failure Statements

Quxlang has four statement forms for deliberate failure: `ASSERT` checks an
invariant, `PANIC` terminates a reached execution path, `COMPILATION_ERROR`
rejects compilation, and `UNIMPLEMENTED` defers its handling to the target
configuration.

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

`ASSERT` always evaluates its condition. It is a language statement, not a
debug-only facility removed from optimized builds.

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

`UNIMPLEMENTED` takes no message in the current source grammar. Its meaning is
selected by the target's `unimplemented_mode`:

| Mode | Result when code generation reaches the statement |
| --- | --- |
| `error` | compilation fails |
| `trap` | an unimplemented terminator is emitted for runtime handling |

The statement marks a deliberately incomplete path. It is not a default value,
an implicit return, or a way to suppress type checking in surrounding code.
See [The `qxcbuild.yml` File](toolchain/qxcbuild-file.md) for the target option.

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
