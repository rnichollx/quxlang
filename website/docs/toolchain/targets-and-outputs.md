# Targets and Outputs

The top-level keys in `quxbuild.yaml` are target names:

```yaml
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
    app:
      type: executable
      main_module: app
      backend_llvm_options:
        mode: optimize
```

## Target identity

The current target-level keys are:

| Key | Accepted current values or shape |
| --- | --- |
| `platform` | `linux`, `windows`, `macos`, or `jvm` |
| `cpu` | `x64`, `x86`, `ARM32`, `ARM64`, `z_arch`, or `jvm` |
| `binary` | `elf`, `macho`, `pe`, or `wasm` on a native target |
| `environment` | `glibc`, `musl`, `bionic`, `msvc`, `ucrt`, `cygwin`, `static`, `libsystem`, or `freestanding` |
| `backend` | `llvm` or `cortado` |
| `backend_llvm_options` | LLVM defaults for the target |
| `backend_cortado_options` | Cortado defaults for the target |
| `unimplemented_mode` | `trap` or `error` for reached `UNIMPLEMENTED` statements |
| `run_static_tests` | Boolean control for source static-test execution |
| `steppings` | Ordered native CPU stepping sequence |
| `modules` | Logical-to-source module map |
| `outputs` | Named output map |

A native target requires `platform` and `cpu`. Its platform establishes the
usual defaults: Linux uses ELF with the `static` environment, Windows uses PE
with `msvc`, and macOS uses Mach-O with `libsystem`. `binary` and `environment`
can override those defaults with one of the accepted values.

A JVM target may select `jvm` through `platform` or `cpu` and defaults to the
Cortado backend. It cannot configure `binary`, `environment`, or native CPU
steppings. Mixing the JVM platform with a native CPU, or a native platform with
the JVM CPU, is rejected.

Schema acceptance does not make every platform/CPU/binary cross-product
linkable. Current final artifact paths cover Linux/ELF, macOS/Mach-O,
Windows/PE, and Cortado/JVM executables and unit-test suites. Other accepted
format values must still have a matching backend and linker path.

These fields supply the source predicates documented on
[Availability and targets](../language/availability-and-targets.md).

## Module mappings

The key under `modules` is the logical name visible to source imports. `source`
names a directory under the bundle's `modules/` tree. Different targets may map
one logical name to different source modules. When `source` is omitted, it
defaults to the logical module name.

`RUNTIME` is the target's runtime module. Source outside it refers to that
logical module with `RUNTIME_MODULE` where a direct absolute reference is
required.

An `options` map supplies the logical module's declared
[build options](../language/build-options.md).

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

The configuration schema also recognizes `shared_library`, `static_library`,
and `image` output kinds. Current LLVM and Cortado artifact generation does not
emit those kinds, so they are reserved rather than usable output forms.

If `outputs` is omitted, the compiler uses a `default` executable rooted at the
logical `main` module and its `::main#()` functanoid.

## Backend modes

Backend options can be defaults on the target or overrides on one output:

```yaml
backend_llvm_options:
  mode: optimize
outputs:
  app-debug:
    type: executable
    main_module: app
    backend_llvm_options:
      mode: debug
```

LLVM accepts `optimize` and `debug`. Cortado accepts `standard` and
`address_sanitizer` under `backend_cortado_options`. The choice is checked into
the source bundle rather than supplied as an ambient command-line optimization
flag.

CPU stepping syntax is documented on
[CPU capabilities and steppings](cpu-capabilities-and-steppings.md), and direct
compiler invocation is documented on [Compiler output](compiler-output.md).
