# Compiler Output and Diagnostics

The direct compiler interface takes an input bundle and output directory:

```console
qxc ./bundle ./out
```

The compiler reads `./bundle/quxbuild.yaml`, loads its configured targets, and
writes their outputs below `./out/output/<target-name>`.

## Debug compiler artifacts

```console
qxc ./bundle ./out --debug-compile-output
```

`--debug-compile-output` writes compiler-stage artifacts below `./out/build` for
diagnosis. The current public command line does not take target-name filters;
one invocation processes the targets loaded from the bundle.

## Reproducibility boundary

Artifact-affecting choices belong to `quxbuild.yaml`: module versions, target
architecture, hosted environment, CPU steppings, module options, outputs, and
backend mode. The compiler does not search for installed source libraries or
accept ambient optimization flags that silently redefine the configured
artifact.

Source-level failures use the declarations and statements documented under
[Tests](../language/tests.md) and
[Diagnostics and explicit failure](../language/diagnostics-and-failure.md).
