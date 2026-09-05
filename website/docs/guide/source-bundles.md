# Source Bundles and Targets

A Quxlang build is described by a source bundle. The compiler does not discover
installed libraries or accept optimization flags that silently change an
artifact; modules, target properties, options, and output settings belong to the
bundle configuration.

## Directory shape

A minimal bundle follows this layout:

```text
example-bundle/
├── qxcbuild.yml
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
    backend_llvm_options:
      mode: optimize
```

The target name is `linux-x64`. Its logical `app`, `std`, and `RUNTIME` modules
resolve to source directories with the same names here, but a target may map a
logical name to a versioned or platform-specific source module.

The `linux-x64/app` output key is the exact path below `./out/output`, and its
`target` field selects `linux-x64`. An executable output selects a main module. A `unit_test_suite` output instead
lists modules whose `UNIT_TEST` declarations should be collected.

## Compile the configured targets

```console
qxc ./example-bundle ./out
```

This invocation compiles every configured target and writes artifacts to
`./out/output/<output-key>`. Add a comma-separated target list, such as
`qxc ./example-bundle ./out linux-x64`, to compile selected targets. Use
`--debug-compile-output` when compiler-stage artifacts are needed for diagnosis;
it does not replace target configuration.

See [Source files and imports](../reference/source-files-and-imports.md),
[the `qxcbuild.yml` overview](../overview/qxcbuild-file.md),
[Build options](../reference/build-options.md), and
[Availability and target selection](../reference/availability-and-targets.md).
