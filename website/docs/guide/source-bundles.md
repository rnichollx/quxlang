# Source Bundles and Targets

A Quxlang build is described by a source bundle. The compiler does not discover
installed libraries or accept optimization flags that silently change an
artifact; modules, target properties, options, and output settings belong to the
bundle configuration.

## Directory shape

A minimal bundle follows this layout:

```text
example-bundle/
├── quxbuild.yaml
└── modules/
    ├── app/
    │   └── sources/
    │       └── main.qxs
    ├── runtime/
    │   └── sources/
    │       └── ...
    └── std/
        └── sources/
            └── ...
```

The directories under `modules/` are source modules. A target maps logical
module names used by source code to those source modules.

## Target and output configuration

```yaml
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
    app:
      type: executable
      main_module: app
      backend_llvm_options:
        mode: optimize
```

The target name is `linux-x64`. Its logical `app`, `std`, and `RUNTIME` modules
resolve to source directories with the same names here, but a target may map a
logical name to a versioned or platform-specific source module.

An executable output selects a main module. A `unit_test_suite` output instead
lists modules whose `UNIT_TEST` declarations should be collected.

## Compile the configured targets

```console
qxc ./example-bundle ./out
```

The current public invocation compiles every target loaded from the bundle and
writes final artifacts below the output directory. Use
`--debug-compile-output` when compiler-stage artifacts are needed for diagnosis;
it does not replace target configuration.

See [Source files and imports](../language/source-files-and-imports.md),
[Build options](../language/build-options.md), and
[Availability and target selection](../language/availability-and-targets.md).
