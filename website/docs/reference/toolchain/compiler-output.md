# Compiler Output and Diagnostics

The direct compiler interface takes an input bundle and output directory:

```console
qxc ./bundle ./out
```

The compiler reads `./bundle/qxcbuild.yml`, loads its configured targets, and
writes each output to `./out/output/<output-key>`. The output mapping key is
its exact relative path; the compiler adds no filename extension.

To compile selected targets, supply their names separated by commas:

```console
qxc ./bundle ./out linux-x64,macos-arm64
```

Without a target list, all configured targets are compiled. Unknown target
names are errors. Configuration validation covers the entire manifest before
selection. Targets without outputs still run their enabled static tests.

## Debug compiler artifacts

```console
qxc ./bundle ./out --debug-compile-output
```

`--debug-compile-output` writes compiler-stage artifacts below
`./out/build/<target>` for diagnosis and can be combined with target selection.
Output-specific intermediates retain the output key's directory structure
beneath that target's build directory. Shared routine artifacts remain grouped
by target.

## Reproducibility boundary

Artifact-affecting choices belong to `qxcbuild.yml`: module versions, target
architecture, hosted environment, CPU steppings, module options, outputs, and
backend mode. The compiler does not search for installed source libraries or
accept ambient optimization flags that silently redefine the configured
artifact.

Source-level failures use the declarations and statements documented under
[Tests](../tests.md) and
[Diagnostics and explicit failure](../diagnostics-and-failure.md).
