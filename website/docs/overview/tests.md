# Overview of Tests

Tests are named declarations with one of three execution modes:

```quxlang
::compile_time_test STATIC_TEST
{
  TEST_ASSERT(clamp(@value -5, @minimum 0, @maximum 10) == 0);
}

::runtime_test UNIT_TEST
{
  TEST_ASSERT(native_operation() == 1);
}

::both_modes_test DUAL_TEST
{
  TEST_ASSERT(clamp(@value 15, @minimum 0, @maximum 10) == 10);
}
```

- `STATIC_TEST` executes during compilation.
- `UNIT_TEST` is collected into a configured runtime unit-test suite.
- `DUAL_TEST` exercises both supported paths.

## Expected static failures

Only `STATIC_TEST` accepts expectation modifiers:

```quxlang
::expected_assertion STATIC_TEST EXPECT_FAIL
{
  TEST_ASSERT(FALSE);
}

::expected_compile_error STATIC_TEST EXPECT_COMPILATION_FAILURE
{
  missing_symbol();
}
```

`EXPECT_FAIL` expects static execution to fail. `EXPECT_COMPILATION_FAILURE`
expects semantic compilation or lowering of the test body to fail.

Use source tests for source-language behavior so the same fixture exercises the
language path. A `unit_test_suite` output lists the logical modules whose
`UNIT_TEST` declarations it collects.

See [Diagnostics and explicit failure](diagnostics-and-failure.md).

## Unconditional test checks

`TEST_ASSERT(condition[, "tag"])` always checks its condition and uses the
assertion failure path. `TEST_EXPECT(condition[, "tag"])` always checks its
condition and throws `TEST_FAILED` on failure. Ordinary `ASSERT` follows
`ASSERT_ENABLED` and may be omitted in a runtime output. See
[Failure Statements](diagnostics-and-failure.md).

## Reference

See the [Tests Reference](../reference/tests.md) for the complete
language rules, constraints, and technical edge cases.
