# Quxlang CPU capability identifiers

## Status and scope

This document is the normative registry of Quxlang CPU capability identifier
stems. Stable identifiers are accepted in CPU stepping configuration and by
the parser's CPU-attribute expressions and symbols. Runtime detection and
code generation remain incomplete.

Future interfaces may prepend an operation to a complete stem. For example,
`HAVE_X64_FEATURE_AVX2` and `DETECT_X64_PERF_FAST_GATHER` are derived from
`X64_FEATURE_AVX2` and `X64_PERF_FAST_GATHER`. This document does not define
the meaning of `HAVE`, `DETECT`, or any other future operation.

Stable identifiers are permanent and must not be renamed or reused. An
identifier beginning with `EXPERIMENTAL_` may be renamed or removed when its
specification changes.

## Stepping configuration

A target may define an ordered `steppings` sequence in `quxbuild.yaml`. The
sequence index is the stepping ID, and later compatible entries have higher
priority. Every entry contains `attributes` in one of two equivalent forms:

```yaml
steppings:
  - attributes:
      - X64_FEATURE_SSE2
  - attributes:
      X64_FEATURE_AVX2: true
      X64_PERF_FAST_GATHER: false
```

An attribute in the sequence form is required. In the object form, `true`
requires an attribute and `false` rejects a machine having that attribute.
Only stable identifiers belonging to the target CPU are accepted. Stepping 0
cannot reject attributes because bootstrap code executes at stepping 0. When
`steppings` is omitted, the target requests the compiler-provided default set;
selection of that default set is not yet implemented.

Executable main-program modules expose `MAIN_FUNCTION_ARRAY`, a fixed-size
array of `PROCEDURE(: I32)` pointers indexed by stepping ID. Each
stepping-specific module contributes the same array definition, with entries
targeting the corresponding `_X<stepping>` main function. Bootstrap runtime
code currently calls element 0; runtime stepping selection remains deferred.

## Identifier grammar

```text
<CPU>_FEATURE_<FEATURE>
<CPU>_PERF_<PROPERTY>
EXPERIMENTAL_<CPU>_FEATURE_<FEATURE>
EXPERIMENTAL_<CPU>_PERF_<PROPERTY>
```

`<CPU>` is exactly one of `X86`, `X64`, `ARM32`, `ARM64`, `RISCV32`,
`RISCV64`, or `Z_ARCH`.

Identifiers use upper-case ASCII letters, decimal digits, and underscores.
Punctuation separating meaningful components becomes an underscore: SSE4.1
becomes `SSE4_1`, AVX10.2-512 becomes `AVX10_2_512`, and Armv8.5-A becomes
`V8_5A`. Punctuation within a canonical extension token is removed: RISC-V
Zve32x becomes `ZVE32X`.

Architecture-standard terminology takes precedence over backend terminology.
Arm's `FEAT_` wrapper is omitted. Published acronyms and extension tokens are
preferred; a published descriptive facility name is used when none exists.
There is one identifier per capability. Consequently, the registry contains
`X64_FEATURE_SSE4_1` and `X64_FEATURE_SSE4_2`, but not the ambiguous
`X64_FEATURE_SSE4`.

Representative complete identifiers are:

- `X64_FEATURE_AVX2`, `X64_FEATURE_SSE4_1`, and
  `X64_FEATURE_SSE4_2`;
- `ARM64_FEATURE_ADVANCED_SIMD`;
- `RISCV64_FEATURE_ZBA`;
- `Z_ARCH_FEATURE_VECTOR_ENHANCEMENTS_1`;
- `X64_PERF_FAST_GATHER`; and
- `EXPERIMENTAL_RISCV64_FEATURE_ZIBI`.

## Categories

`FEATURE` identifies a software-visible architectural capability. Enabling it
changes the legal instruction set, architectural registers or behavior, or
another facility visible to software. Mandatory baseline features, privileged
facilities, published vendor extensions, and standardized architecture levels
or bundles may be named.

`PERF` identifies a microarchitectural fact that may change a cost model
without changing instruction legality or program semantics. Fusion, latency,
throughput, false dependencies, and fast or slow implementations belong here.
A performance property may require a feature, but never implies that feature.

The following do not receive identifiers:

- CPU models and processor aliases;
- ABI, endian, execution-mode, floating-point ABI, and register-reservation
  choices;
- optimizer preferences and policies such as `prefer-*`, `allow-*`,
  `disable-*`, `use-*`, outline-atomics, and forced atomics;
- assembler, linker-relaxation, exact-assembly, and save/restore policies;
- mitigation, hardening, and erratum-workaround controls;
- object-format conventions such as tagged globals; and
- negative aliases for positive capabilities.

LLVM mappings below are non-normative backend notes. Another backend can
implement the same identifier using a different representation.

## Registry notation

A row listing CPUs `X86, X64` and token `AES` defines both
`X86_FEATURE_AES` and `X64_FEATURE_AES`. Brace lists and ranges are expanded
exhaustively: `CDE_CP{0..7}` defines only `CDE_CP0` through `CDE_CP7`.

Unless overridden with `TOKEN=llvm-name`, an LLVM mapping is `+` followed by
the lower-case token with underscores changed to hyphens. `Specification`
means that the prerequisites and implications are those defined by the named
architecture specification.

## X86 and X64

### Architectural features

Each token defines `<CPU>_FEATURE_<TOKEN>` for every CPU in its row.

| CPUs | Tokens or mappings | Prerequisites and notes |
| --- | --- | --- |
| X86, X64 | `ADX`, `AES`, `AVX`, `AVX2`, `AVX512BF16`, `AVX512BITALG`, `AVX512BW`, `AVX512CD`, `AVX512DQ`, `AVX512F`, `AVX512FP16`, `AVX512IFMA`, `AVX512VBMI`, `AVX512VBMI2`, `AVX512VL`, `AVX512VNNI`, `AVX512VP2INTERSECT`, `AVX512VPOPCNTDQ` | Specification. |
| X86, X64 | `AVX10_1=avx10.1`, `AVX10_1_512=avx10.1-512`, `AVX10_2=avx10.2`, `AVX10_2_512=avx10.2-512`, `AVXIFMA`, `AVXNECONVERT`, `AVXVNNI`, `AVXVNNIINT8`, `AVXVNNIINT16` | The `_512` forms additionally permit 512-bit vectors. |
| X86, X64 | `BMI1=bmi`, `BMI2`, `BRANCH_HINT=branch-hint`, `CCMP`, `CF`, `CLDEMOTE`, `CLFLUSHOPT`, `CLWB`, `CLZERO`, `CMOV`, `CMPCCXADD`, `CRC32`, `CMPXCHG8B=cx8`, `ENQCMD`, `EVEX512=evex512` | Scalar, cache, conditional, queueing, encoding, and checksum capabilities. |
| X86, X64 | `F16C`, `FMA`, `FMA4`, `GFNI`, `HRESET`, `INVPCID`, `LWP`, `LZCNT`, `MMX`, `MOVBE`, `MOVDIR64B`, `MOVDIRI`, `MOVRS`, `MWAITX`, `NOPL`, `POPCNT` | Specification. |
| X86, X64 | `PCLMULQDQ=pclmul`, `PCONFIG`, `PKU`, `PREFETCHI`, `PREFETCHW=prfchw`, `PTWRITE`, `RAOINT`, `RDPID`, `RDPRU`, `RDRAND=rdrnd`, `RDSEED`, `RTM`, `SERIALIZE`, `SGX` | Specification. |
| X86, X64 | `SHA`, `SHA512`, `SM3`, `SM4`, `SSE=sse`, `SSE2`, `SSE3`, `SSSE3`, `SSE4_1=sse4.1`, `SSE4_2=sse4.2`, `SSE4A`, `TBM`, `TSXLDTRK`, `UINTR`, `USERMSR`, `VAES`, `VPCLMULQDQ`, `WAITPKG`, `WBNOINVD`, `X87`, `XOP` | SSE4.1 and SSE4.2 are distinct. |
| X86, X64 | `FXSAVE_FXRSTOR=fxsr`, `XSAVE`, `XSAVEC`, `XSAVEOPT`, `XSAVES`, `UNALIGNED_SSE_MEMORY=sse-unaligned-mem` | State-management facilities; unaligned SSE memory requires `SSE`. |
| X64 | `CMPXCHG16B=cx16`, `FSGSBASE`, `LAHF_SAHF=sahf`, `CET_SHADOW_STACK=shstk` | Capabilities specific to 64-bit mode in the architectural or LLVM definition. |
| X64 | `AMX_TILE=amx-tile`, `AMX_AVX512=amx-avx512`, `AMX_BF16=amx-bf16`, `AMX_COMPLEX=amx-complex`, `AMX_FP16=amx-fp16`, `AMX_FP8=amx-fp8`, `AMX_INT8=amx-int8`, `AMX_MOVRS=amx-movrs`, `AMX_TF32=amx-tf32` | AMX components require `AMX_TILE`. |
| X64 | `APX_EGPR=egpr`, `APX_NDD=ndd`, `APX_NF=nf`, `APX_PPX=ppx`, `APX_PUSH2_POP2=push2pop2`, `APX_ZERO_UPPER=zu` | Intel APX components. |
| X64 | `KEY_LOCKER=kl`, `KEY_LOCKER_WIDE=widekl` | The wide form requires Key Locker. |
| X64 | `V2=x86-64-v2`, `V3=x86-64-v3`, `V4=x86-64-v4` | Standard x86-64 psABI levels; these map to LLVM CPU baselines. |

### Performance properties

Each token defines `X86_PERF_<TOKEN>` and `X64_PERF_<TOKEN>`.

| Tokens or mappings | Meaning and prerequisites |
| --- | --- |
| `BRANCH_FUSION=branchfusion`, `MACRO_FUSION=macrofusion` | The corresponding instruction classes fuse. |
| `ENHANCED_REP_MOVSB_STOSB=ermsb`, `FAST_SHORT_REP_MOVSB=fsrm` | REP-string performance properties. |
| `FALSE_DEPENDENCY_GETMANT=false-deps-getmant`, `FALSE_DEPENDENCY_LZCNT_TZCNT=false-deps-lzcnt-tzcnt`, `FALSE_DEPENDENCY_MULC=false-deps-mulc`, `FALSE_DEPENDENCY_MULLQ=false-deps-mullq`, `FALSE_DEPENDENCY_PERMUTE=false-deps-perm`, `FALSE_DEPENDENCY_POPCNT=false-deps-popcnt`, `FALSE_DEPENDENCY_RANGE=false-deps-range` | The named instruction class has a false destination dependency and requires its corresponding instruction feature. |
| `FAST_7_BYTE_NOP=fast-7bytenop`, `FAST_11_BYTE_NOP=fast-11bytenop`, `FAST_15_BYTE_NOP=fast-15bytenop` | Maximum efficiently decoded NOP length. |
| `FAST_BEXTR=fast-bextr`, `FAST_DPWSSD=fast-dpwssd`, `FAST_GATHER=fast-gather`, `FAST_HORIZONTAL_OPS=fast-hops`, `FAST_IMMEDIATE_16=fast-imm16`, `FAST_IMMEDIATE_VECTOR_SHIFT=tuning-fast-imm-vector-shift`, `FAST_LZCNT=fast-lzcnt`, `FAST_MOVBE=fast-movbe` | Fast implementation of the named operation; the corresponding feature is a prerequisite. |
| `FAST_SCALAR_SQRT=fast-scalar-fsqrt`, `FAST_VECTOR_SQRT=fast-vector-fsqrt`, `FAST_SCALAR_SHIFT_MASK=fast-scalar-shift-masks`, `FAST_VECTOR_SHIFT_MASK=fast-vector-shift-masks`, `FAST_SHLD_ROTATE=fast-shld-rotate`, `FAST_VARIABLE_CROSS_LANE_SHUFFLE=fast-variable-crosslane-shuffle`, `FAST_VARIABLE_PER_LANE_SHUFFLE=fast-variable-perlane-shuffle`, `SHIFT_FASTER_THAN_SHUFFLE=faster-shift-than-shuffle` | Relative execution-cost facts. |
| `LEA_USES_ADDRESS_GENERATION=lea-uses-ag`, `NO_BYPASS_DELAY=no-bypass-delay`, `NO_BYPASS_DELAY_BLEND=no-bypass-delay-blend`, `NO_BYPASS_DELAY_MOVE=no-bypass-delay-mov`, `NO_BYPASS_DELAY_SHUFFLE=no-bypass-delay-shuffle`, `SBB_DEPENDENCY_BREAKING=sbb-dep-breaking` | Pipeline-resource and dependency facts. |
| `SLOW_3_OPERAND_LEA=slow-3ops-lea`, `SLOW_INC_DEC=slow-incdec`, `SLOW_LEA=slow-lea`, `SLOW_PMADDWD=slow-pmaddwd`, `SLOW_PMULLD=slow-pmulld`, `SLOW_PMULLQ=slow-pmullq`, `SLOW_SHLD=slow-shld`, `SLOW_TWO_MEMORY_OPERANDS=slow-two-mem-ops`, `SLOW_UNALIGNED_MEMORY_16=slow-unaligned-mem-16`, `SLOW_UNALIGNED_MEMORY_32=slow-unaligned-mem-32` | Slow implementation of the named operation. |

## ARM32

### Architectural features

Each token defines `ARM32_FEATURE_<TOKEN>`.

| Group | Tokens or mappings | Prerequisites |
| --- | --- | --- |
| Levels | `V4=armv4`, `V4T=armv4t`, `V5T=armv5t`, `V5TE=armv5te`, `V5TEJ=armv5tej`, `V6=armv6`, `V6J=armv6j`, `V6K=armv6k`, `V6KZ=armv6kz`, `V6M=armv6-m`, `V6S_M=armv6s-m`, `V6T2=armv6t2`, `V7A=armv7-a`, `V7M=armv7-m`, `V7R=armv7-r`, `V7EM=armv7e-m`, `V7K=armv7k`, `V7S=armv7s`, `V7VE=armv7ve`, `V8A=armv8-a`, `V8M_BASE=armv8-m.base`, `V8M_MAIN=armv8-m.main`, `V8R=armv8-r`, `V8_1A=armv8.1-a`, `V8_1M_MAIN=armv8.1-m.main`, `V8_{2..9}A=armv8.{2..9}-a`, `V9A=armv9-a`, `V9_{1..7}A=armv9.{1..7}-a` | Each level implies its published predecessor and mandatory set. LLVM's shorter `v*` attributes are aliases, not additional identifiers. |
| Profiles and core | `A_PROFILE=aclass`, `R_PROFILE=rclass`, `M_PROFILE=mclass`, `ACQUIRE_RELEASE=acquire-release`, `ATOMICS_32=atomics-32`, `DATA_BARRIER=db`, `FULL_DATA_BARRIER=dfb`, `DSP`, `HARDWARE_DIVIDE_THUMB=hwdiv`, `HARDWARE_DIVIDE_ARM=hwdiv-arm`, `MP`, `THUMB2`, `TRUSTZONE`, `V7_CLREX=v7clrex`, `V8M_SECURITY=8msecext`, `VIRTUALIZATION` | Specification. |
| Floating point | `D32=d32`, `FP=fp-armv8`, `FP_ARMV8_D16=fp-armv8d16`, `FP_ARMV8_D16_SP=fp-armv8d16sp`, `FP_ARMV8_SP=fp-armv8sp`, `FP16=fullfp16`, `FP16_CONVERSION=fp16`, `FP16FML`, `FP64`, `FPAO`, `FP_REGISTERS=fpregs`, `FP_REGISTERS_16=fpregs16`, `FP_REGISTERS_64=fpregs64` | Narrow and single-precision forms do not imply wider alternatives. |
| VFP | `VFP2`, `VFP2_SP=vfp2sp`, `VFP3`, `VFP3_D16=vfp3d16`, `VFP3_D16_SP=vfp3d16sp`, `VFP3_SP=vfp3sp`, `VFP4`, `VFP4_D16=vfp4d16`, `VFP4_D16_SP=vfp4d16sp`, `VFP4_SP=vfp4sp` | Versioned forms imply corresponding earlier subsets. |
| Vector and matrix | `NEON=neon`, `MVE=mve`, `MVE_FP=mve.fp`, `BF16`, `DOTPROD`, `I8MM` | `MVE_FP` requires `MVE`; specification otherwise applies. |
| Crypto and system | `AES`, `CRC32=crc`, `CRYPTO`, `SHA2`, `CDE`, `CDE_CP{0..7}=cdecp{0..7}`, `CLRBHB`, `LOB`, `PACBTI`, `PMUV3=perfmon`, `RAS`, `SB` | `CRYPTO` is a standard bundle; each `CDE_CPn` requires `CDE`. |
| Vendor ISA | `IWMMXT`, `IWMMXT2`, `XSCALE=xscale` | Published vendor extensions. |

### Performance properties

Each token defines `ARM32_PERF_<TOKEN>`.

| Tokens or mappings | Meaning and prerequisites |
| --- | --- |
| `CHEAP_PREDICABLE_CPSR=cheap-predicable-cpsr`, `MOVS_SHIFTED_OPERAND_EXPENSIVE=avoid-movs-shop`, `MULS_EXPENSIVE=avoid-muls`, `PARTIAL_CPSR_UPDATE_EXPENSIVE=avoid-partial-cpsr`, `MUXED_AGU_NEON_FPU=muxed-units`, `NO_BRANCH_PREDICTOR=no-branch-predictor`, `NONPIPELINED_VFP=nonpipelined-vfp`, `RETURN_ADDRESS_STACK=ret-addr-stack` | Pipeline and cost facts. The `avoid-*` LLVM mappings express optimizer choices derived from the neutrally named cost facts. |
| `FUSE_AES=fuse-aes`, `FUSE_LITERALS=fuse-literals`, `VMLX_FORWARDING=vmlx-forwarding`, `VMLX_HAZARDS=vmlx-hazards`, `ZERO_CYCLE_ZEROING=zcz` | Fusion, forwarding, hazard, and zeroing facts. |
| `MVE_1_BEAT=mve1beat`, `MVE_2_BEAT=mve2beat`, `MVE_4_BEAT=mve4beat` | Mutually exclusive MVE throughput models; requires `ARM32_FEATURE_MVE`. |
| `SLOW_FP_COMPARE_BRANCH=slow-fp-brcc`, `SLOW_LOAD_D_SUBREGISTER=slow-load-D-subreg`, `SLOW_ODD_REGISTER=slow-odd-reg`, `SLOW_VDUP32=slow-vdup32`, `SLOW_VGETLNI32=slow-vgetlni32`, `SLOW_FP_FMA=slowfpvfmx`, `SLOW_FP_MAC=slowfpvmlx` | Slow implementation of the named operation. |

## ARM64

### Architectural features

Each token defines `ARM64_FEATURE_<TOKEN>`. Names are Arm architecture feature
names with the `FEAT_` wrapper removed.

| Group | Tokens or mappings | Prerequisites |
| --- | --- | --- |
| Levels | `V8A=v8a`, `V8R=v8r`, `V8_{1..9}A=v8.{1..9}a`, `V9A=v9a`, `V9_{1..7}A=v9.{1..7}a` | Each level implies its published predecessor and mandatory set. |
| Arithmetic and crypto | `FP=fp-armv8`, `ADVANCED_SIMD=neon`, `AES`, `SHA2`, `CRYPTO`, `CRC32=crc`, `PMUV3=perfmon`, `RDM`, `SM4`, `SHA3`, `FP16=fullfp16`, `I8MM=i8mm`, `F32MM=f32mm`, `F64MM=f64mm`, `FHM=fp16fml`, `DOTPROD=dotprod`, `BF16`, `FCMA=complxnum`, `JSCVT=jsconv`, `FRINTTS=fptoint` | Specification; `CRYPTO` is a standard bundle. |
| Atomic and ordering | `LSE`, `LSE2`, `LSE128`, `LRCPC=rcpc`, `LRCPC2=rcpc-immo`, `LRCPC3=rcpc3`, `LS64`, `LSFE`, `LSCP`, `D128` | Numbered versions imply predecessors. |
| System and control | `CSV2_2=specrestrict`, `PAN`, `PAN2=pan-rwv`, `LOR`, `CONTEXTIDR_EL2=CONTEXTIDREL2`, `VHE=vh`, `RAS`, `RASV2=rasv2`, `SPE`, `SPEV1P2=spe-eef`, `UAO=uaops`, `DPB=ccpp`, `DPB2=ccdp`, `PAUTH=pauth`, `PAUTH_LR=pauth-lr`, `FPAC`, `CCIDX`, `NV`, `MPAM`, `MPAMV2=mpamv2`, `DIT`, `TRF=tracev8.4`, `AMUV1=am`, `AMUV1P1=amvs`, `SEL2`, `TLBIOS_TLBIRANGE=tlb-rmi`, `FLAGM=flagm`, `FLAGM2=altnzcv`, `SB`, `SSBS`, `SPECRES=predres`, `BTI=bti`, `BTIE=btie`, `RNG=rand`, `MTE`, `MTETC`, `FGT`, `ECV`, `XS`, `WFXT=wfxt`, `HCX`, `NMI`, `CLRBHB`, `PRFMSLC=prfm-slc-target`, `SPECRES2`, `THE`, `TRBE`, `ETE`, `BRBE`, `RME`, `MEC`, `CHK`, `GCS`, `ITE`, `CPA`, `TLBIW`, `CMH`, `TLBID`, `GCIE`, `EL2VMSA`, `EL3` | Specification; enhancements require their base capability. |
| SVE | `SVE`, `SVE2`, `SVE2P1=sve2p1`, `SVE2P2=sve2p2`, `SVE2P3=sve2p3`, `SVE_AES=sve-aes`, `SVE_AES2=sve-aes2`, `SVE_SM4=sve-sm4`, `SVE_SHA3=sve-sha3`, `SVE_BITPERM=sve-bitperm`, `SVE_B16B16=sve-b16b16`, `SVE_B16MM=sve-b16mm`, `SVE_BFSCALE=sve-bfscale`, `SVE_F16F32MM=sve-f16f32mm` | `SVE2` requires `SVE`; numbered versions imply predecessors. LLVM `sve2-*` aliases are not identifiers. |
| SME and streaming SVE | `SME`, `SME2`, `SME2P1=sme2p1`, `SME2P2=sme2p2`, `SME2P3=sme2p3`, `SME_B16B16=sme-b16b16`, `SME_F16F16=sme-f16f16`, `SME_F64F64=sme-f64f64`, `SME_I16I64=sme-i16i64`, `SME_FA64=sme-fa64`, `SME_LUTV2=sme-lutv2`, `SME_F8F16=sme-f8f16`, `SME_F8F32=sme-f8f32`, `SME_MOP4=sme-mop4`, `SME_TMOP=sme-tmop`, `SSVE_AES=ssve-aes`, `SSVE_BITPERM=ssve-bitperm`, `SSVE_FEXPA=ssve-fexpa`, `SSVE_FP8DOT2=ssve-fp8dot2`, `SSVE_FP8DOT4=ssve-fp8dot4`, `SSVE_FP8FMA=ssve-fp8fma` | `SME2` requires `SME`; specification component prerequisites apply. |
| Later extensions | `HBC`, `MOPS`, `MOPS_GO=mops-go`, `CSSC`, `FAMINMAX`, `LUT`, `FP8`, `FP8FMA`, `FP8DOT2`, `FP8DOT4`, `F8F16MM`, `F8F32MM`, `FPRCVT`, `F16MM`, `F16F32DOT`, `F16F32MM`, `OCCMO`, `PCDPHINT`, `POPS=pops`, `LSUI`, `CMPBR`, `TEV`, `S1POE2=poe2` | FP8 and vector/matrix component prerequisites apply. |

### Performance properties

Each token defines `ARM64_PERF_<TOKEN>`.

| Tokens or mappings | Meaning and prerequisites |
| --- | --- |
| `SLOW_ADDRESS_LSL_1_4=addr-lsl-slow-14`, `FAST_ALU_LSL_0_4=alu-lsl-fast`, `CHEAP_AS_MOVE_HANDLING=exynos-cheap-as-move`, `SELECT_EXPENSIVE=predictable-select-expensive` | Relative execution-cost facts. The Exynos-named LLVM mapping does not make the Quxlang property vendor-specific. |
| `ARITHMETIC_BCC_FUSION=arith-bcc-fusion`, `ARITHMETIC_CBZ_FUSION=arith-cbz-fusion`, `CMP_BCC_FUSION=cmp-bcc-fusion`, `FUSE_ADDRESS=fuse-address`, `FUSE_ADDSUB_TWO_REGISTER_CONSTANT_ONE=fuse-addsub-2reg-const1`, `FUSE_ADRP_ADD=fuse-adrp-add`, `FUSE_AES=fuse-aes`, `FUSE_ARITHMETIC_LOGIC=fuse-arith-logic`, `FUSE_CRYPTO_EOR=fuse-crypto-eor`, `FUSE_CSEL=fuse-csel`, `FUSE_CSET=fuse-cset`, `FUSE_LITERALS=fuse-literals` | The named operations fuse; corresponding instruction features are prerequisites. |
| `SLOW_MISALIGNED_128_STORE=slow-misaligned-128store`, `SLOW_PAIRED_128=slow-paired-128`, `SLOW_STRQ_REGISTER_OFFSET_STORE=slow-strqro-store` | Slow memory-operation forms. |
| `ZERO_CYCLE_MOVE_FPR128=zcm-fpr128`, `ZERO_CYCLE_MOVE_FPR64=zcm-fpr64`, `ZERO_CYCLE_MOVE_FPR32=zcm-fpr32`, `ZERO_CYCLE_MOVE_GPR64=zcm-gpr64`, `ZERO_CYCLE_MOVE_GPR32=zcm-gpr32` | Zero-cycle register moves. |
| `ZERO_CYCLE_ZEROING_FPR128=zcz-fpr128`, `ZERO_CYCLE_ZEROING_FPR64` (inverse of `no-zcz-fpr64`), `ZERO_CYCLE_ZEROING_GPR64=zcz-gpr64`, `ZERO_CYCLE_ZEROING_GPR32=zcz-gpr32` | Zero-cycle register zeroing. |

## RISCV32 and RISCV64

### Architectural features

Each stable token defines both `RISCV32_FEATURE_<TOKEN>` and
`RISCV64_FEATURE_<TOKEN>` unless the CPUs column narrows it.

| CPUs | Group | Tokens | Prerequisites |
| --- | --- | --- | --- |
| RISCV32, RISCV64 | Base | `I`, `E`, `M`, `A`, `F`, `D`, `Q`, `C`, `B`, `V`, `H` | Specification and XLEN restrictions; `B` is the Zba/Zbb/Zbs bundle. |
| RISCV32, RISCV64 | Integer and memory | `ZA64RS`, `ZA128RS`, `ZAAMO`, `ZABHA`, `ZACAS`, `ZALASR`, `ZALRSC`, `ZAMA16B`, `ZAWRS`, `ZBA`, `ZBB`, `ZBC`, `ZBS`, `ZIC64B`, `ZICBOM`, `ZICBOP`, `ZICBOZ`, `ZICCAMOA`, `ZICCAMOC`, `ZICCIF`, `ZICCLSM`, `ZICCRSE`, `ZICOND`, `ZICSR`, `ZIFENCEI`, `ZILSD`, `ZIMOP`, `ZMMUL`, `ZTSO` | Specification and XLEN restrictions. |
| RISCV32, RISCV64 | Hints and counters | `ZICNTR`, `ZIHINTNTL`, `ZIHINTPAUSE`, `ZIHPM` | Specification. |
| RISCV32, RISCV64 | Floating point | `ZDINX`, `ZFA`, `ZFBFMIN`, `ZFH`, `ZFHMIN`, `ZFINX`, `ZHINX`, `ZHINXMIN` | Full forms imply corresponding minimal forms. |
| RISCV32, RISCV64 | Compressed | `ZCA`, `ZCB`, `ZCD`, `ZCE`, `ZCF`, `ZCLSD`, `ZCMOP`, `ZCMP`, `ZCMT` | Specification and XLEN restrictions. |
| RISCV32, RISCV64 | Scalar crypto | `ZK`, `ZKN`, `ZKND`, `ZKNE`, `ZKNH`, `ZKR`, `ZKS`, `ZKSED`, `ZKSH`, `ZKT`, `ZBKB`, `ZBKC`, `ZBKX` | Standard bundles imply their published components. |
| RISCV32, RISCV64 | Vector | `ZVE32X`, `ZVE32F`, `ZVE64X`, `ZVE64F`, `ZVE64D`, `ZVBB`, `ZVBC`, `ZVFBFMIN`, `ZVFBFWMA`, `ZVFH`, `ZVFHMIN`, `ZVKB`, `ZVKG`, `ZVKN`, `ZVKNC`, `ZVKNED`, `ZVKNG`, `ZVKNHA`, `ZVKNHB`, `ZVKS`, `ZVKSC`, `ZVKSED`, `ZVKSG`, `ZVKSH`, `ZVKT`, `ZVL{32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536}B` | Specification; larger minimum vector lengths imply smaller lengths. |
| RISCV32, RISCV64 | Privileged | `SDEXT`, `SDTRIG`, `SHA`, `SHCOUNTERENW`, `SHGATPA`, `SHLCOFIDELEG`, `SHTVALA`, `SHVSATPA`, `SHVSTVALA`, `SHVSTVECD`, `SMAIA`, `SMCDELEG`, `SMCNTRPMF`, `SMCSRIND`, `SMCTR`, `SMDBLTRP`, `SMEPMP`, `SMMPM`, `SMNPM`, `SMRNMI`, `SMSTATEEN`, `SSAIA`, `SSCCFG`, `SSCCPTR`, `SSCOFPMF`, `SSCOUNTERENW`, `SSCSRIND`, `SSCTR`, `SSDBLTRP`, `SSNPM`, `SSPM`, `SSQOSID`, `SSSTATEEN`, `SSSTRICT`, `SSTC`, `SSTVALA`, `SSTVECD`, `SSU64XL`, `SUPM`, `SVADE`, `SVADU`, `SVBARE`, `SVINVAL`, `SVNAPOT`, `SVPBMT`, `SVVPTC` | Privileged specification. |
| RISCV32 | Profiles | `RVI20U32` | Published profile set. |
| RISCV64 | Profiles | `RVA20U64`, `RVA20S64`, `RVA22U64`, `RVA22S64`, `RVA23U64`, `RVA23S64`, `RVB23U64`, `RVB23S64`, `RVI20U64` | Published profile sets. |
| RISCV32, RISCV64 | Andes | `XANDESBFHCVT`, `XANDESPERF`, `XANDESVBFHCVT`, `XANDESVDOT`, `XANDESVPACKFPH`, `XANDESVSINTH`, `XANDESVSINTLOAD` | Published vendor extensions. |
| RISCV32, RISCV64 | CORE-V and MIPS | `XCVALU`, `XCVBI`, `XCVBITMANIP`, `XCVELW`, `XCVMAC`, `XCVMEM`, `XCVSIMD`, `XMIPSCBOP`, `XMIPSCMOV`, `XMIPSEXECTL`, `XMIPSLSP` | Published vendor extensions. |
| RISCV32, RISCV64 | Qualcomm | `XQCCMP`, `XQCI`, `XQCIA`, `XQCIAC`, `XQCIBI`, `XQCIBM`, `XQCICLI`, `XQCICM`, `XQCICS`, `XQCICSR`, `XQCIINT`, `XQCIIO`, `XQCILB`, `XQCILI`, `XQCILIA`, `XQCILO`, `XQCILSM`, `XQCISIM`, `XQCISLS`, `XQCISYNC` | Published vendor extensions. |
| RISCV32, RISCV64 | SiFive | `XSFCEASE`, `XSFMM128T`, `XSFMM16T`, `XSFMM32A16F`, `XSFMM32A32F`, `XSFMM32A8F`, `XSFMM32A8I`, `XSFMM32T`, `XSFMM64A64F`, `XSFMM64T`, `XSFMMBASE`, `XSFVCP`, `XSFVFBFEXP16E`, `XSFVFEXP16E`, `XSFVFEXP32E`, `XSFVFEXPA`, `XSFVFEXPA64E`, `XSFVFNRCLIPXFQF`, `XSFVFWMACCQQQ`, `XSFVQMACCDOD`, `XSFVQMACCQOQ`, `XSIFIVECDISCARDDLONE`, `XSIFIVECFLUSHDLONE` | Published vendor extensions. |
| RISCV32, RISCV64 | Other vendors | `XSMTVDOT`, `XTHEADBA`, `XTHEADBB`, `XTHEADBS`, `XTHEADCMO`, `XTHEADCONDMOV`, `XTHEADFMEMIDX`, `XTHEADMAC`, `XTHEADMEMIDX`, `XTHEADMEMPAIR`, `XTHEADSYNC`, `XTHEADVDOT`, `XVENTANACONDOPS`, `XWCHC` | Published vendor extensions. |

These experimental tokens define
`EXPERIMENTAL_RISCV32_FEATURE_<TOKEN>` and
`EXPERIMENTAL_RISCV64_FEATURE_<TOKEN>`. Their LLVM mapping is
`+experimental-` followed by the lower-case token:

`P`, `RVM23U32`, `SMPMPMT`, `SVUKTE`, `XRIVOSVISNI`, `XRIVOSVIZIP`,
`XSFMCLIC`, `XSFSCLIC`, `ZIBI`, `ZICFILP`, `ZICFISS`, `ZVBC32E`,
`ZVFBFA`, `ZVFOFP8MIN`, `ZVKGS`, and `ZVQDOTQ`.

### Performance properties

Each token defines both `RISCV32_PERF_<TOKEN>` and
`RISCV64_PERF_<TOKEN>`.

| Tokens or mappings | Meaning and prerequisites |
| --- | --- |
| `ADD_LOAD_FUSION=add-load-fusion`, `ADDI_LOAD_FUSION=addi-load-fusion`, `AUIPC_ADDI_FUSION=auipc-addi-fusion`, `AUIPC_LOAD_FUSION=auipc-load-fusion`, `BITFIELD_EXTRACT_FUSION=bfext-fusion`, `CONDITIONAL_CMOV_FUSION=conditional-cmv-fusion`, `LD_ADD_FUSION=ld-add-fusion`, `LUI_ADDI_FUSION=lui-addi-fusion`, `LUI_LOAD_FUSION=lui-load-fusion`, `SHIFTED_ZEXTW_FUSION=shifted-zextw-fusion`, `SHXADD_LOAD_FUSION=shxadd-load-fusion`, `ZEXTH_FUSION=zexth-fusion`, `ZEXTW_FUSION=zextw-fusion` | The named operations fuse; instruction features are prerequisites. |
| `DLEN_HALF_VLEN=dlen-factor-2`, `LOGARITHMIC_VRGATHER_LATENCY=log-vrgather`, `SELECT_EXPENSIVE=predictable-select-expensive`, `SINGLE_ELEMENT_VECTOR_FP64=single-element-vec-fp64`, `VECTOR_LENGTH_DEPENDENT_LATENCY=vl-dependent-latency`, `VXRM_WRITE_PIPELINE_FLUSH=vxrm-pipeline-flush` | Pipeline and relative execution-cost facts. |
| `OPTIMIZED_NF{2..8}_SEGMENT_LOAD_STORE=optimized-nf{2..8}-segment-load-store`, `OPTIMIZED_ZERO_STRIDE_LOAD=optimized-zero-stride-load` | Optimized vector memory implementations. |
| `SHORT_FORWARD_BRANCH_IALU=short-forward-branch-ialu`, `SHORT_FORWARD_BRANCH_ILOAD=short-forward-branch-iload`, `SHORT_FORWARD_BRANCH_IMINMAX=short-forward-branch-iminmax`, `SHORT_FORWARD_BRANCH_IMUL=short-forward-branch-imul` | Short-forward-branch performance properties. |
| `FAST_UNALIGNED_SCALAR_MEMORY=unaligned-scalar-mem`, `FAST_UNALIGNED_VECTOR_MEMORY=unaligned-vector-mem` | Unaligned accesses have reasonable performance; vector form requires a vector feature. |

## Z architecture

### Architectural features

Each token defines `Z_ARCH_FEATURE_<TOKEN>`.

| Tokens or mappings | Meaning and prerequisites |
| --- | --- |
| `BEAR_ENHANCEMENT=bear-enhancement`, `CONCURRENT_FUNCTIONS=concurrent-functions`, `DEFLATE_CONVERSION=deflate-conversion`, `DFP_PACKED_CONVERSION=dfp-packed-conversion`, `DFP_ZONED_CONVERSION=dfp-zoned-conversion`, `DISTINCT_OPERANDS=distinct-ops`, `ENHANCED_DAT_2=enhanced-dat-2`, `ENHANCED_SORT=enhanced-sort`, `EXECUTION_HINT=execution-hint`, `FAST_SERIALIZATION=fast-serialization`, `FLOATING_POINT_EXTENSION=fp-extension`, `GUARDED_STORAGE=guarded-storage`, `HIGH_WORD=high-word` | Same-named IBM facilities. |
| `INSERT_REFERENCE_BITS_MULTIPLE=insert-reference-bits-multiple`, `INTERLOCKED_ACCESS_1=interlocked-access1`, `LOAD_AND_TRAP=load-and-trap`, `LOAD_AND_ZERO_RIGHTMOST_BYTE=load-and-zero-rightmost-byte`, `LOAD_STORE_ON_CONDITION=load-store-on-cond`, `LOAD_STORE_ON_CONDITION_2=load-store-on-cond-2` | Same-named facilities; numbered enhancements require the base. |
| `MESSAGE_SECURITY_ASSIST_EXTENSION_{3,4,5,7,8,9,12}=message-security-assist-extension{3,4,5,7,8,9,12}` | Exact numbered Message-Security-Assist facilities. |
| `MISCELLANEOUS_EXTENSIONS=miscellaneous-extensions`, `MISCELLANEOUS_EXTENSIONS_{2,3,4}=miscellaneous-extensions-{2,3,4}`, `NNP_ASSIST=nnp-assist`, `POPULATION_COUNT=population-count`, `PROCESSOR_ACTIVITY_INSTRUMENTATION=processor-activity-instrumentation`, `PROCESSOR_ASSIST=processor-assist`, `RESET_DAT_PROTECTION=reset-dat-protection`, `RESET_REFERENCE_BITS_MULTIPLE=reset-reference-bits-multiple`, `TEST_PENDING_EXTERNAL_INTERRUPTION=test-pending-external-interruption`, `TRANSACTIONAL_EXECUTION=transactional-execution` | Same-named IBM facilities. |
| `VECTOR=vector`, `VECTOR_ENHANCEMENTS_{1,2,3}=vector-enhancements-{1,2,3}`, `VECTOR_PACKED_DECIMAL=vector-packed-decimal`, `VECTOR_PACKED_DECIMAL_ENHANCEMENT=vector-packed-decimal-enhancement`, `VECTOR_PACKED_DECIMAL_ENHANCEMENT_{2,3}=vector-packed-decimal-enhancement-{2,3}` | Vector enhancements require their published predecessors and `VECTOR`. |

LLVM 22.1.6 exposes no backend-neutral SystemZ performance property.
`backchain`, `soft-float`, and `unaligned-symbols` are ABI or code-generation
policies and do not define `Z_ARCH_PERF_*` identifiers.

## LLVM 22.1.6 audit boundary

This registry was audited against `llc -mattr=help` for `i386`, `x86_64`,
`armv7`, `aarch64`, `riscv32`, `riscv64`, and `s390x`, and against the
corresponding LLVM TableGen feature and processor-model files.

An unmapped LLVM attribute is intentionally excluded if it belongs to a policy
class above. In particular:

- x86 mode, ABI, hardening, retpoline, padding, divide-rewrite, `prefer-*`,
  and `use-*` flags are excluded;
- ARM processor names, frame-chain/thread-pointer choices, register
  reservation, execution-only/strict-alignment/long-call policies, hardening,
  errata, and instruction-selection preferences are excluded;
- RISC-V processor names, reservation, relaxation/exact-assembly,
  save/restore, scheduling, forced-atomics, permissive-Zalrsc, and
  instruction-selection controls are excluded; and
- SystemZ ABI and code-generation controls are excluded.


## Primary naming sources

- Intel, [*Intel 64 and IA-32 Architectures Software Developer's Manual* and
  related instruction-set extension specifications](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html).
- Arm, [architecture feature identifiers and feature
  tables](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a).
- RISC-V International, [ratified and development ISA
  specifications](https://riscv.org/specifications/ratified/).
- IBM, [*z/Architecture Principles of
  Operation*](https://www.ibm.com/docs/en/systems-hardware/zsystems/3932-AGZ?topic=library-2).
- LLVM 22.1.6 target TableGen files and processor scheduling models, used only
  for LLVM mappings and the audited backend surface.
