# Overview of Build Options

An `OPTION` declaration exposes a value supplied by the target's module
configuration.

```quxlang
::retry_count OPTION NUMBER DEFAULT(3);
::tracing_enabled OPTION BOOL DEFAULT(FALSE);
::output_name OPTION STRING DEFAULT("quxlang-output");
::inherited_retry_count OPTION NUMBER DEFAULT_FROM(::retry_count);
```

The supported kinds are `NUMBER`, `BOOL`, and `STRING`.

## Defaults

- `DEFAULT(expression)` supplies a default expression.
- `DEFAULT_VALUE(expression)` is an accepted equivalent spelling.
- `DEFAULT_FROM(symbol)` inherits another option's default. The referenced
  declaration must resolve to an option of the same kind.

An option may omit a source default when every relevant target supplies it.

## Target values

The `options` map belongs to a logical module mapping:

```yaml
modules:
  tests:
    source: tests
    options:
      retry_count: 5
      tracing_enabled: true
      output_name: instrumented-tests
```

The configured value becomes a compile-time value in that logical module. A
target may therefore select behavior without rewriting source and without
turning a build option into a runtime global.

## Use an option

Options are read-only compile-time values:

```quxlang
STATIC_IF(tracing_enabled)
{
  emit_trace();
}

VAR attempts I32 := retry_count AS I32;
```

The first use selects generated code. The second converts a numeric option for
ordinary runtime use. Source code cannot assign a new value to an option.

Use [`INCLUDE_IF`](availability-and-targets.md) when an option controls whether
a declaration exists, or `STATIC_IF` when it selects code inside a declaration.

## Reference

See the [Build Options Reference](../reference/build-options.md) for the complete
language rules, constraints, and technical edge cases.
