# The `qxcbuild.yml` File

Every source bundle must contain `qxcbuild.yml` at its root. The document requires two top-level
mappings: `targets` for compilation settings and `outputs` for artifact paths:

```yaml
targets:
  linux-x64:
    platform: linux
    cpu: x64
    environment: glibc
    backend: llvm
    modules:
      RUNTIME:
        source: runtime
      std:
        source: std
      app:
        source: app
        options:
          tracing_enabled: false
outputs:
  linux-x64/app:
    target: linux-x64
    type: executable
    main_module: app
    backend_llvm_options:
      mode: optimize
```

The filename is exact. The loader does not search for alternate `.yaml` names
or a manifest in a parent directory. The root value and each target, module,
output, backend-options, and stepping value use the mapping or sequence shape
specified below. Unknown fields are rejected rather than ignored. Field names
and enumerated string values are case-sensitive.

## Top-level sections

Only `targets` and `outputs` are accepted at the root. Each target key names a
compilation configuration. Each output key is a globally unique, exact path
relative to `<qxc-output>/output`, and its required `target` field selects a
configured target. Neither a separate output name nor a `path` field is used.

An empty `outputs: {}` mapping is valid. A target with no outputs still runs
its enabled static tests and emits no implicit binary. The former root-level
target format and target-nested `outputs` are rejected.

## Target identity

The current target-level keys are:

| Key | Accepted current values or shape | Default or requirement |
| --- | --- | --- |
| `platform` | `linux`, `windows`, `macos`, or `jvm` | Required for a native target |
| `cpu` | `x64`, `x86`, `ARM32`, `ARM64`, `z_arch`, or `jvm` | Required for a native target |
| `binary` | `elf`, `macho`, `pe`, or `wasm` on a native target | Derived from `platform` |
| `environment` | `glibc`, `musl`, `bionic`, `msvc`, `ucrt`, `cygwin`, `static`, `libsystem`, or `freestanding` | Derived from `platform` |
| `backend` | `llvm` or `cortado` | `llvm` for native; `cortado` for JVM |
| `backend_llvm_options` | LLVM defaults for the target | `mode: optimize` |
| `backend_cortado_options` | Cortado defaults for the target | `mode: standard` |
| `unimplemented_mode` | `trap` or `error` for reached `UNIMPLEMENTED` statements | `trap` |
| `run_static_tests` | Boolean control for source static-test execution | `true` |
| `steppings` | Ordered native CPU stepping sequence | Compiler-selected when omitted |
| `modules` | Logical-to-source module map | Needed for every logical module used by an output |

A native target requires `platform` and `cpu`. Its platform establishes the
usual defaults: Linux uses ELF with the `static` environment, Windows uses PE
with `msvc`, and macOS uses Mach-O with `libsystem`. `binary` and `environment`
can override those defaults with one of the accepted values.

A JVM target may select `jvm` through `platform` or `cpu` and defaults to the
Cortado backend. It cannot configure `binary`, `environment`, or native CPU
steppings. Mixing the JVM platform with a native CPU, or a native platform with
the JVM CPU, is rejected.

LLVM options are invalid on a Cortado target, and Cortado options are invalid
on an LLVM target. The LLVM backend cannot target the JVM; Cortado currently
requires the JVM.

Schema acceptance does not make every platform/CPU/binary cross-product
linkable. Current final artifact paths cover Linux/ELF, macOS/Mach-O,
Windows/PE, and Cortado/JVM executables and unit-test suites. Other accepted
format values must still have a matching backend and linker path.

These fields supply the source predicates documented on
[Availability and targets](../availability-and-targets.md).

## Module mappings

The key under `modules` is the logical name visible to source imports. `source`
names a directory under the bundle's `modules/` tree. Different targets may map
one logical name to different source modules. When `source` is omitted, it
defaults to the logical module name.

Each module mapping accepts only `source` and `options`:

```yaml
modules:
  app:
    source: app_2_0
    options:
      tracing_enabled: true
      retry_count: 4
```

The source directory must exist in the bundle and contain a `sources/`
directory. Option names are resolved against `OPTION` declarations in the
logical module, and their scalar values must match the declared option kinds.

`RUNTIME` is the target's runtime module. Source outside it refers to that
logical module with `RUNTIME_MODULE` where a direct absolute reference is
required.

An `options` map supplies the logical module's declared
[build options](../build-options.md).

## Outputs

The two currently artifact-producing output forms are:

- `executable`, which selects `main_module` and normally its `::main`
  functanoid; and
- `unit_test_suite`, which lists `test_modules` whose `UNIT_TEST` declarations
  are collected.

An executable can name `main_functanoid` explicitly when it needs an entry
other than `::main#()`. `main_module` is valid only for an executable;
`test_modules` is valid only for a unit-test suite; and a unit-test suite cannot
set `main_functanoid`.

Every output mapping requires `target` and `type` and accepts only `main_module`,
`test_modules`, `main_functanoid`, `backend_llvm_options`, and
`backend_cortado_options` in addition to those fields. For an executable,
`main_module` defaults to `main` and `main_functanoid` defaults to `::main#()`.
For a unit-test suite, `test_modules` defaults to `[main]`; an explicit list
must be nonempty, contain no duplicates, and name configured logical modules.

The configuration schema also recognizes `shared_library`, `static_library`,
and `image` output kinds. Current LLVM and Cortado artifact generation does not
emit those kinds, so they are reserved rather than usable output forms.

Output keys must be nonempty, normalized relative file paths using `/` between
components. Absolute paths, `.` or `..` components, repeated separators,
trailing separators, nonportable filename characters, Windows device names,
and components longer than 255 bytes are rejected. Components cannot end in a
space or period. Paths cannot duplicate another output after ASCII case folding
or use another output's file path as a directory.

The key is literal: use `windows/app.exe` or `java/app.jar` when those extensions
are wanted. The compiler creates parent directories and never appends a suffix.
Outputs can share a directory with distinct filenames, or use the same filename
in different directories. All configuration entries, including output paths
and target references, are validated before target filtering.

## Backend modes

Backend options can be defaults on the target or overrides on one output:

```yaml
targets:
  linux-x64:
    platform: linux
    cpu: x64
    backend_llvm_options:
      mode: optimize
    modules:
      RUNTIME:
        source: runtime
      app:
        source: app
outputs:
  linux-x64/app-debug:
    target: linux-x64
    type: executable
    main_module: app
    backend_llvm_options:
      mode: debug
```

LLVM accepts `optimize` and `debug`. Cortado accepts `standard` and
`address_sanitizer` under `backend_cortado_options`. The choice is checked into
the source bundle rather than supplied as an ambient command-line optimization
flag.

An output override replaces the corresponding target-level backend settings
for that output. Backend settings for the other backend are invalid rather
than ignored.

CPU stepping syntax is documented on
[CPU capabilities and steppings](cpu-capabilities-and-steppings.md), and direct
compiler invocation is documented on [Compiler output](compiler-output.md).

!!! warning "JVM Backend"
    Binaries produced by the JVM backend have no optimizations (even if optimizations are enabled) and 
    extremely poor performance. Future work may improve this, but the JVM backend code is at usually 
    around 100x slower than native code, which tends to be competitive with or beat `gcc -O2`.
