# Overview of Compilation Policies

Compilation policies let each output choose assertion and safety checks.
For example, a release test suite can retain bounds checks:

```yaml
outputs:
  app/tests:
    target: native
    type: unit_test_suite
    test_modules: [tests]
    build_type: Release
    policies:
      policy_check_bounds: true
```

Use `POLICY` when source behavior must follow the same setting:

```quxlang
POLICY ASSERT_ENABLED
{
  TEST_ASSERT(expensive_invariant());
}
```

An optional `ELSE` supplies the disabled alternative. Available policies are
`ASSERT_ENABLED`, `CHECK_BOUNDS`, `CHECK_OVERFLOW`, and `UNIMPLEMENTED_PANICS`.
Constant evaluation enables all four. Ordinary `ASSERT` may be omitted at
runtime; use `TEST_ASSERT` or `TEST_EXPECT` for test checks that always run.

The [reference](../reference/compilation-policies.md) lists build-type defaults,
configuration keys, and each policy's behavior.
