# Toolchain

`qxc` treats the compiler as the build system for a complete source bundle. The
bundle identifies every source module, logical module mapping, target property,
build option, and output that can affect the artifact.

- [The `qxcbuild.yml` File](qxcbuild-file.md) specifies the source-bundle
  manifest, targets, module mappings, and outputs.
- [CPU capabilities and steppings](cpu-capabilities-and-steppings.md) describes
  portable feature identifiers and multiversioned outputs.
- [Program startup and runtime hooks](program-startup-and-runtime-hooks.md)
  describes application dispatch, runtime entrypoints, and test tables.
- [Runtime module contracts](runtime-module-contracts.md) describes the
  reserved allocator, diagnostics, initialization, and thread-lifecycle
  declarations supplied by `RUNTIME`.
- [Backends and layout](backends-and-layout.md) distinguishes native and
  layoutless targets.
- [Compiler output and diagnostics](compiler-output.md) describes direct `qxc`
  invocation and generated artifact directories.
- [Source bundles](../../guide/source-bundles.md) gives the introductory directory
  layout.

The bundle declares its configured targets, and the current public `qxc`
invocation compiles the targets loaded from that bundle. The command line does
not act as an untracked second build configuration. This is central to
deterministic, reproducible compilation.
