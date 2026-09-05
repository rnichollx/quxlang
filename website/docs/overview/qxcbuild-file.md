# Overview of the `qxcbuild.yml` File

Every Quxlang source bundle has a `qxcbuild.yml` file at its root. The file
names the targets to compile, maps logical module names to source directories,
and describes the artifacts each target should produce.

## Configure a minimal executable

```yaml
targets:
  linux-x64:
    platform: linux
    cpu: x64
    backend: llvm
    modules:
      RUNTIME:
        source: runtime
      std:
        source: std
      app:
        source: app
outputs:
  linux-x64/app:
    target: linux-x64
    type: executable
    main_module: app
```

`linux-x64` is the target name. Its `platform`, `cpu`, and `backend` select the
machine and compiler backend. Each entry under `modules` creates a logical name
that source files can import; `source` names the corresponding directory below
the bundle's `modules/` directory.

The `linux-x64/app` output selects the `linux-x64` target and uses the logical
`app` module's entry point. Its key is the exact artifact path below the output
directory: `qxc ./bundle ./out` writes `./out/output/linux-x64/app`. Unless
configured otherwise, an executable uses `::main#()`.

Both top-level sections are required. Use `outputs: {}` to run enabled static
tests without producing binaries.

## Configure build options and several outputs

```yaml
targets:
  linux-x64:
    platform: linux
    cpu: x64
    environment: glibc
    modules:
      RUNTIME:
        source: runtime
      app:
        source: app
        options:
          tracing_enabled: true
      tests:
        source: tests
outputs:
  linux-x64/app-release:
    target: linux-x64
    type: executable
    main_module: app
    backend_llvm_options:
      mode: optimize
  linux-x64/tests:
    target: linux-x64
    type: unit_test_suite
    test_modules:
    - RUNTIME
    - app
    - tests
```

Module `options` supply values for declarations made with `OPTION`. Output-level
backend settings can override target defaults. A `unit_test_suite` collects the
`UNIT_TEST` declarations from its `test_modules` instead of selecting a main
function.

## Add another target

Entries under `targets` are independent, so the same bundle can map modules
differently for another machine. Each output selects one of those targets:

```yaml
targets:
  jvm:
    platform: jvm
    cpu: jvm
    modules:
      RUNTIME:
        source: runtime
      app:
        source: app
outputs:
  jvm/app.jar:
    target: jvm
    type: executable
    main_module: app
```

The `jvm/app.jar` output includes its `.jar` extension explicitly. Output keys
are exact paths; the compiler does not add platform extensions.

A JVM target defaults to the Cortado backend. Native targets require both a
native platform and CPU. Keep platform-specific declarations behind target
predicates so each configured target sees a valid program.

See [Source Bundles and Targets](../guide/source-bundles.md) for the directory
layout and compiler invocation, and [Build Options](build-options.md)
for declaring values supplied by a module mapping.

## Reference

See [The `qxcbuild.yml` File Reference](../reference/toolchain/qxcbuild-file.md)
for the complete schema, defaults, accepted values, field restrictions, and
currently supported output forms.
