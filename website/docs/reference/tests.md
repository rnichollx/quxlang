# Tests

Quxlang tests are named declarations whose mode determines whether their body
executes during compilation, in a generated unit-test suite, or in both paths.

## Declaration forms

```quxlang
::compile_time_test STATIC_TEST
{
  ASSERT(2 + 2 == 4);
}

::runtime_test UNIT_TEST
{
  ASSERT(native_operation() == 1);
}

::both_modes_test DUAL_TEST
{
  ASSERT(clamp(@value 15, @minimum 0, @maximum 10) == 10);
}
```

A test has no parameter list or declared return type. Its body is a function
block, and the declaration does not end with a semicolon.

## `STATIC_TEST`

`STATIC_TEST` is evaluated by the compiler. A successful test must compile,
execute its selected compile-time path, and finish without an assertion,
explicit failure, panic, or other static-evaluation failure.

Static tests are appropriate for source-language semantics, compile-time
algorithms, overload selection, and invariants that do not require a hosted
runtime.

## `UNIT_TEST`

`UNIT_TEST` is collected into configured `unit_test_suite` output. Its body is
compiled as runtime code and invoked by the generated test table. Merely
building a library module does not independently execute every unit test; the
selected output controls collection and execution.

Target availability still applies. A test excluded by `INCLUDE_IF` is not
present in that target's active test set.

## `DUAL_TEST`

`DUAL_TEST` exercises the same body through both the supported static and
runtime test paths. It is useful for language operations intended to have the
same observable behavior in compile-time evaluation and native execution.

A dual test must satisfy the restrictions of both paths. Runtime-only behavior
should use `UNIT_TEST`; a compile-time-only contract should use `STATIC_TEST`.

## Expected static failures

Only `STATIC_TEST` accepts an expectation modifier, written after the test kind
and before the body.

`EXPECT_FAIL` requires compilation to reach static execution and for that
execution to fail:

```quxlang
::expected_assertion STATIC_TEST EXPECT_FAIL
{
  ASSERT(FALSE);
}
```

`EXPECT_COMPILATION_FAILURE` requires semantic compilation or lowering of the
test body to fail:

```quxlang
::expected_compile_error STATIC_TEST EXPECT_COMPILATION_FAILURE
{
  missing_symbol();
}
```

The distinction is intentional. An assertion or `PANIC` reached by the static
interpreter is an execution failure, whereas an unknown name, invalid overload,
or prohibited lifetime transition is a compilation failure. Using either
modifier on `UNIT_TEST` or `DUAL_TEST` is a syntax error.

An expected-failure test fails the test run when the anticipated failure does
not occur or occurs in the wrong phase.

## Naming and collection

Tests use ordinary named declaration and privacy rules. A descriptive name is
part of test output and should state the behavior being verified. Associated
tests may be declared inside types where that ownership is meaningful.

The runtime module exposes compiler-owned unit-test table contracts such as the
test count, names, and procedure entries. Programs normally consume those
through the generated suite rather than declaring their own replacements. See
[Program Startup and Runtime Hooks](toolchain/program-startup-and-runtime-hooks.md).

Use `.qxs` source tests for language behavior so parser, semantic analysis,
lowering, and execution follow the same public language path. See
[Failure Statements](diagnostics-and-failure.md) for `ASSERT`,
`PANIC`, and compilation-error statements.
