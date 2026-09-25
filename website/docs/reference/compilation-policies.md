# Compilation Policies

A compilation policy selects behavior when code is lowered for an output.
Different outputs can select different policies for the same source.

## Source syntax

```quxlang
POLICY CHECK_BOUNDS
{
  TEST_ASSERT(index < count);
}
ELSE
{
  use_unchecked_index(@index index);
}
```

`POLICY` accepts `ASSERT_ENABLED`, `CHECK_BOUNDS`, `CHECK_OVERFLOW`, or
`UNIMPLEMENTED_PANICS`, followed by a block and an optional `ELSE` block.
Both alternatives undergo source semantic analysis; lowering selects one.
Use `STATIC_IF` when a condition must discard source before semantic generation.

| Policy | Enabled behavior |
| --- | --- |
| `ASSERT_ENABLED` | Evaluate `ASSERT` and diagnose a false condition. Disabled assertions omit condition evaluation. |
| `CHECK_BOUNDS` | Check array indexing and array element-address bounds. Disabling checks leaves the validity requirement in force. |
| `CHECK_OVERFLOW` | Check assumed-in-range arithmetic (`+!`, `-!`, `*!`, `/!` and corresponding shifts/rotations); violations panic. |
| `UNIMPLEMENTED_PANICS` | A permitted `UNIMPLEMENTED` panics when reached. When disabled, a lowering-reachable statement fails compilation. |

Explicit checked arithmetic such as `+?` throws `ARITHMETIC_OVERFLOW`
independently of policy. `TEST_ASSERT` and `TEST_EXPECT` also remain active
independently of assertion policy. See [Arithmetic Operators](arithmetic-operators.md)
and [Failure Statements](diagnostics-and-failure.md).

## Output configuration

Set Boolean overrides under an output's `policies` mapping in `qxcbuild.yml`:

```yaml
outputs:
  app/tests:
    target: native
    type: unit_test_suite
    test_modules: [tests]
    build_type: Release
    policies:
      policy_assert_enabled: true
      policy_check_bounds: true
      policy_check_overflow: true
      policy_unimplemented_panics: false
```

The target `native` and module `tests` must be configured in the same bundle.
Unknown or repeated policy names are rejected.

| Effective build type | Assertions, bounds, overflow checks | Unimplemented panics |
| --- | --- | --- |
| `Development`, `Debug`, `DebugOpt`, `DebugCompact`, `DebugCompactOpt` | Enabled | Enabled |
| `Quick`, `Compact`, `CompactOpt` | Disabled | Enabled |
| `Release`, `ReleaseDbgSym` | Disabled | Disabled |

Build type names ignore ASCII case and underscores. The default target build
type is `Development`. An output can override it; for LLVM, the effective LLVM
`build_type` determines policy defaults. Explicit policy entries override these
defaults. Constant evaluation enables all four policies independently of output
overrides.

`unimplemented_compiles` is a separate target-level Boolean, defaulting to
`true`. Setting it to `false` rejects `UNIMPLEMENTED` during code generation,
before output policy selection. See [Build Configuration](qxcbuild-file.md).
