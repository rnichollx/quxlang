# Windows exception handling handoff

## Implementation and artifact contract

Exception handling uses the Quxlang unwinder in `MODULE(RUNTIME)` on every native target, including glibc Linux and Windows. Windows execution testing remains outstanding. The Mac/Linux implementation has been exercised as described below.

Qxc is a hermetic, reproducible cross compiler. Compilation must not consume installed SDKs, libraries, unwind archives or host-dependent object files to produce artifact contents. System services use Quxlang `EXTERN_PROCEDURE` declarations or compiler-generated calls. Neither libunwind nor libgcc is an unwinder dependency. An external compatibility layer can be considered separately when interoperability requires one.

Windows output uses LLVM DWARF exception tables and the Quxlang frame walker. The PE linker retains `.eh_frame` input as `.ehframe` and resolves `quxlang_unwind_begin` and `quxlang_unwind_end` to its boundaries. The implementation does not use Windows SEH dispatch, `.pdata` or `.xdata` for Quxlang propagation. Runtime cursor and propagation structures are internal types, not Windows `CONTEXT` records. Any future types matching platform binary layouts must use `IBC_STRUCT`.

The Windows register routines follow the x64 calling convention. They preserve RBX, RBP, RSI, RDI, R12–R15 and all 128 bits of XMM6–XMM15. RAX carries the exception record and RDX the landing-pad selector. The upper halves of preserved XMM registers occupy separate slots in the internal context. Verify this on real Windows execution, especially when cleanup calls preserve live floating-point/vector values.

## Language and runtime contract

- A TRY owns a nullable `EXCEPTION_PTR` local. VMIR protects blocks with `CATCH %exception_local, !handler`; existing local lifetime state determines cleanup.
- Typed catches bind `CONST& T` or `MUT& T`. Operations on `EXCEPTION_PTR` implement ordinary source-ordered branches. Native unwind tables identify cleanup/handler boundaries, not language type dispatch.
- `exception_frame` contains `ATOMIC#SZ refcount`, an object pointer, an object-operations interface, and an allocation pointer. The interface reports actual type, exposes a polymorphic view when supported, and destroys the complete allocation.
- `EXCEPTION_PTR` retains a frame. Presence is tested with the postfix ?? operator; IS_OUT_OF_MEMORY() identifies the sentinel. Reference counts are private implementation state, and the type implementation remains in MODULE(RUNTIME). `CURRENT_EXCEPTION()` retains the dynamically active handler's frame or returns an empty handle. A handler guard restores the previous thread-local activation on exit.
- The global `UNWIND_OUT_OF_MEMORY` frame permanently owns one reference. Sentinel/default catches recognize it; typed object catches exclude it. Each throw has an independent mutable propagation record. Four reserved records per thread support propagation-allocation failure; exhaustion terminates without allocating.
- `THROW_EXCEPTION_PTR` adds a propagation reference to an existing handle. Native delivery transfers it into the TRY local. Saved handles preserve object identity and mutation. `EXCEPTION_PROPAGATE` is the legal, runtime-only source intrinsic; double-underscore symbols remain compiler-only.
- Destructors and DEFER are implicitly NOEXCEPT. Internal catches during cleanup are permitted. An escaping exception or an uncaught exception terminates.
- `POLYMORPHIC_BASE` is a compiler built-in type, identifies the complete polymorphic object without stored base subobjects, and exposes `FINAL_TYPE()` for dynamic type reporting. Matching supports exact types and unique permitted dynamic casts.
- Resuming cleanup captures the current call site. It must not skip an enclosing handler when LLVM inlines cleanup and catch scopes into the same physical frame.

## Implementation locations

Runtime sources are under `quxlang/tests/testdata/testbundle/modules/runtime/sources/`:

| File | Responsibility |
| --- | --- |
| `exception.qxs` | Frame ownership, exception pointers, matching, handler activation, payload allocation/destruction |
| `exception_memory.qxs` | Fallible storage allocation and release |
| `exception_unwind.qxs` | Independent propagation records, reserved OOM records, personality and termination |
| `exception_dwarf.qxs` | DWARF decoding, frame recovery, search and cleanup passes |
| `exception_registers.qxs` | Register capture/restoration and section-boundary access |

The compiler defines POLYMORPHIC_BASE and FINAL_TYPE() through the canonical built-in symbol and member-query paths; no runtime source declaration supplies the type.

Compiler integration is in `co_vmir_generator2.hpp`, `ir2_constexpr_interpreter.cpp`, `llvm-backend.cpp`, VMIR dependency queries, and the ELF, Mach-O and PE linkers. Ordinary calling ABIs are unchanged. The backend computes propagation-record field offsets from target layouts rather than hard-coding host offsets.

## Validation completed on macOS

- Release Qxc build succeeds through `local/qxc-build-release.sh`.
- The final API spellings are THROW_EXCEPTION_PTR(), POLYMORPHIC_BASE.FINAL_TYPE(), exception?? and exception.IS_OUT_OF_MEMORY(). EXCEPTION_PTR exposes no reference-count query.
- All 38 focused constexpr cases pass; the std constexpr suite also passes. The built-in POLYMORPHIC_BASE fixture also passes constexpr validation in a standalone bundle with no runtime module, including uppercase FINAL_TYPE() and separation from user-defined lowercase methods.
- All 27 positive exception cases pass on macOS ARM64 and Docker Linux x86, static x86-64, glibc x86-64, ARM64 and s390x. These include concurrent handlers, shared sentinel ownership, partial destruction, saved handles, polymorphic matching and register-array copying.
- Three forced allocation-failure cases pass on every listed Mac/Linux target: repeated zero-allocation throws, propagation-record failure after successful payload allocation, and nested sentinel handling during cleanup.
- Four separate-process termination cases pass on macOS ARM64 and every listed Linux target: uncaught, NOEXCEPT escape, DEFER escape and throwing-subobject-destructor escape.
- The complete Linux x86-64 testbundle compiles and executes all 337 unit tests.
- Repeating the Windows build from the same source bundle and configuration on macOS produces byte-identical output. Cross-host comparison remains outstanding.
- The 27-case Windows x64 suite cross-compiles on macOS with its concurrent-handler test enabled. Artifact inspection confirms `.ehframe` and only declared Windows service imports. Windows execution is untested.
- Dependency inspection finds no dynamic section in static Linux output, only `libc.so.6` in hosted Linux output, and libSystem in Mach-O output. Windows imports kernel32 and synchronization APIs; none of these artifacts imports an external unwinder.

The temporary harnesses and logs reside under `tmp/` on the development host and are not repository prerequisites. Known issues are tracked in [issues-workarounds.md](issues-workarounds.md).

## Remaining Windows validation

1. Execute the focused and full suites, including concurrent handlers. Review `windows_start.qxs`, `thread_runtime.qxs`, `exception_registers.qxs` and `pe_linker.cpp` when diagnosing ABI failures.
2. Test preserved integer and floating-point/vector registers across throws, nested catches and cleanup calls. Inspect `.ehframe` relocations, frame descriptions and linker-provided boundaries.
3. In a temporary runtime copy, add a failure counter to the native branch of `exception_allocate_storage`, returning an empty storage pointer at zero. Use zero for object/frame failure and one for successful payload allocation followed by propagation-record failure. Repeated and nested sentinel catches must restore the permanent frame's reference count after all handles leave scope. Do not add a new allocator or production fault-injection API.
4. Execute uncaught, NOEXCEPT, DEFER and subobject-destructor escape fixtures in separate processes. The corresponding constexpr-negative cases in `main_test_37_exceptions.qxs` identify the intended bodies. Each process must terminate rather than continuing to an outer catch or hanging.
5. Compile an identical copied bundle and target configuration on Windows and macOS/Linux, then compare complete artifact bytes. Configuration, output name, source contents and compiler revision must match. A cross-host byte comparison remains outstanding.
6. Record commands, counts and remaining limitations in this document. Windows behavior remains unverified until the produced programs execute on Windows.

Foreign C++/SEH exceptions and frame discovery in external DLLs/shared libraries are outside the current contract. Do not add an installed unwind library as a build-time workaround. Any interoperability integration requires an explicit ABI bridge and must preserve hermetic artifact construction.
