# Build Options

An `OPTION` declaration exposes a compile-time value supplied by a logical
module's target configuration. Options let targets configure source without
creating mutable runtime globals.

## Declaration grammar

```text
::name OPTION kind [default_clause];
```

The supported kinds are `NUMBER`, `BOOL`, and `STRING`:

```quxlang
::retry_count OPTION NUMBER DEFAULT(3);
::tracing_enabled OPTION BOOL DEFAULT(FALSE);
::output_name OPTION STRING DEFAULT("quxlang-output");
```

The semicolon is required. The configured or default value must match the
declared option kind.

## Direct defaults

`DEFAULT(expression)` and `DEFAULT_VALUE(expression)` are equivalent spellings:

```quxlang
::first OPTION NUMBER DEFAULT(4);
::second OPTION NUMBER DEFAULT_VALUE(4);
```

The expression is evaluated as a compile-time value in the declaration's
context. It is used when the active module configuration does not supply an
override.

An option may omit a source default:

```quxlang
::required_endpoint OPTION STRING;
```

Every target that needs the option must then supply a value. An option with
neither a configured value nor a resolvable default cannot provide a value to
the program.

## Inherited defaults

`DEFAULT_FROM(symbol)` uses another option's effective default:

```quxlang
::base_retry_count OPTION NUMBER DEFAULT(3);
::retry_count OPTION NUMBER DEFAULT_FROM(::base_retry_count);
```

The symbol is resolved in the declaration context and must name an `OPTION` of
the same kind. A `NUMBER` option cannot inherit from a `BOOL` or `STRING`
option. Unresolvable references and cyclic default dependencies are invalid.

The configured value for `retry_count` still overrides its inherited default;
`DEFAULT_FROM` is not an alias that forces the two options to remain equal.

## Target configuration

Option overrides belong to a logical module mapping in `qxcbuild.yml`:

```yaml
modules:
  tests:
    source: tests
    options:
      retry_count: 5
      tracing_enabled: true
      output_name: instrumented-tests
```

The key is the option declaration's name in that logical module. The value is
part of the target's deterministic compilation input. Different target module
mappings may supply different values for the same source declaration.

## Using an option

An option is a read-only compile-time value:

```quxlang
STATIC_IF(tracing_enabled)
{
  emit_trace();
}

VAR attempts I32 := retry_count AS I32;
```

It can participate in static evaluation, target-dependent declaration
selection, constant initialization, and ordinary expressions that accept its
value category. Source code cannot assign to it.

Use [`INCLUDE_IF`](availability-and-targets.md) when the value determines
whether a declaration exists, and [Compile-Time Evaluation](compile-time-evaluation.md)
when it selects generated code inside a declaration.
