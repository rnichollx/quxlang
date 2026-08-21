# CPU Capabilities and Steppings

Quxlang target configuration uses backend-neutral CPU capability identifiers.
A stable identifier follows one of these forms:

```text
<CPU>_FEATURE_<FEATURE>
<CPU>_PERF_<PROPERTY>
<CPU>_VENDOR_<VENDOR>
<CPU>_TUNE_<VENDOR>_<MODEL>
```

Examples include `X64_FEATURE_AVX2`, `X64_FEATURE_SSE4_1`,
`ARM64_FEATURE_ADVANCED_SIMD`, `RISCV64_FEATURE_ZBA`,
`X64_PERF_FAST_GATHER`, and `X64_VENDOR_AMD`. Experimental names begin with
`EXPERIMENTAL_`.

The capability registry includes RISC-V identifiers, but the current public
`quxbuild.yaml` loader does not yet accept a RISC-V target. Those identifiers
therefore describe the stable naming surface for that architecture rather than
a currently emitted public target.

## Stepping configuration

A target can define an ordered series of increasingly specialized steppings:

```yaml
steppings:
  - attributes:
      - X64_FEATURES_V1
  - attributes:
      - X64_FEATURES_V2
  - attributes:
      X64_FEATURES_V3: true
      X64_FEATURE_AVX512F: false
    tune: X64_TUNE_AMD_ZEN1
```

Sequence entries require an attribute. In the map form, `true` requires the
attribute and `false` rejects machines that have it. Later compatible steppings
have higher priority. Stepping zero is the unconditional fallback and cannot
contain negative constraints.

Aggregate x64 levels `X64_FEATURES_V1` through `X64_FEATURES_V4` expand to their
standard constituent capabilities for matching. Requiring an aggregate means
all of its constituent leaves are present. Rejecting it means at least one leaf
is absent.

## Source-level expressions

Source-level `HAVE_<CAPABILITY>` expressions such as
`HAVE_X64_FEATURE_AVX2` are not implemented. Configure the capability on a
stepping instead; the compiler specializes each stepping's generated code for
that stepping's feature set.

Feature and performance attributes affect legality or optimization. `VENDOR`
is a detectable predicate. `TUNE` changes cost and scheduling models but does
not enable instructions or imply a vendor.

At runtime, `ACTIVE_STEPPING` contains the selected stepping index and
`STEPPING_COUNT` contains the number of configured entries. The compiler emits
parallel main and post-detection procedure arrays so each selected entry calls
code compiled for the same stepping. See
[Program startup and runtime hooks](program-startup-and-runtime-hooks.md) for
the complete dispatch contract.

The repository's normative `docs/cpu_capability_identifiers.md` registry lists
the complete current identifier set and its architecture-specific definitions.
