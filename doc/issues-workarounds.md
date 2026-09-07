# Issues and workarounds

This file tracks issues encountered during exception handling implementation. An entry marked resolved requires a successful regression check; a code change alone is not evidence of resolution.

## Private element types in generated array operations

**Status:** Resolved in focused constexpr validation.

Indexing an array whose element type is `PRIVATE(MODULE)` can fail with an inaccessible-type diagnostic from the generated `OPERATOR[]` context, even when the array expression is inside the permitted module.

`instanciation_concrete_params.cpp` re-ran lookup on the entire array parameter while lowering its concrete type. This repeated the access check on an element type that argument initialization had already resolved, using the generated array operation's context instead of the original source context.

The change preserves the resolved element type and evaluates only the array extent expression. The temporary workaround of making the runtime's frame-rule types public has been removed; those declarations are private again. A focused constexpr test passed private array indexing and assignment. Existing privacy checks remain in place; the change only removes repeated lookup of an already-bound array element type.

## Constexpr dynamic-array storage lifecycle

**Status:** Resolved; the std constexpr suite passes.

`MODULE(std)::dynarr_operations_and_lifecycle_static_test` fails with `During constexpr evaluation: storage init start on non-empty storage` in the current exception implementation tree.

The focused native exception bundle temporarily sets `run_static_tests: false` so native unwinder development can proceed independently. Value copying was clearing the destination object's storage-owner association. The interpreter now preserves destination storage ownership and projection metadata. The std constexpr suite, including the originally failing test, passes with this correction.

## Debug artifact generation for expected-failure static tests

**Status:** Open.

The debug compilation path attempts native dependency processing for static-only tests, including ordinary static ABI checks and tests marked `STATIC_TEST EXPECT_FAIL`. It can fail after writing output artifacts, although normal compilation handles the expected constexpr failure.

For temporary inspection, use an isolated debug bundle that declares these bodies as ordinary functions and does not include them as native roots. Retain the original expected-failure tests in the normal constexpr suite. This workaround does not constitute validation of the debug driver.

## Running Qxc while its executable is being relinked

**Status:** Avoided by sequencing validation.

A test harness launched while the compiler build was relinking `qxc` failed because the executable path was temporarily absent. Wait for the build to complete before starting validation, or use an explicitly copied, stable compiler executable and record which build it represents.

## Build change detection during overlapping edits

**Status:** Operational limitation observed during this work.

Editing compiler sources while cbuild is already running can leave the next invocation reporting no changes even though the earlier build did not compile the edit. Keep compiler edits and builds sequential. Runtime source edits can proceed during a C++ build because those files are inputs to subsequent Qxc invocations rather than to that C++ compilation.

## Short-circuit WHILE conditions skip their first operand

**Status:** Resolved in constexpr and macOS/Linux native validation.

`co_generate_statement_ovl(function_while_statement)` passes its condition block by reference while generating a short-circuit expression. That generation advances the block to the last operand, and the loop subsequently uses the advanced block as its back-edge target. For `WHILE (left && right)`, later iterations can therefore skip `left`.

This caused the runtime DWARF instruction reader to continue past its section boundary. The fix preserves the original condition-entry block for both the normal loop back-edge and CONTINUE. Regression tests cover changing left operands with both `&&` and `||`, including CONTINUE. The runtime uses ordinary corrected loop semantics.

## Resume skips enclosing catches after inlining

**Status:** Resolved in macOS ARM64 and Linux x86-64 native validation.

A cleanup landing pad may share a physical frame with an enclosing catch after LLVM inlining. Resuming by advancing the original saved frame skips that catch. The runtime now captures registers at the resume call site and resumes the cleanup walk there. The call-propagation regression and all 25 positive native tests pass.

## Missing runtime dependency mapping in debug output

**Status:** Resolved for the isolated native exception debug bundle.

The debug driver's local dependency switch omitted new exception runtime roles and left an enum uninitialized. It now uses the shared runtime dependency mapping. VMIR, LLVM IR and native artifacts can be emitted for the isolated exception bundle. Expected-failure test handling remains a separate entry above.

## Replacing a running macOS artifact in place

**Status:** Execution workaround.

During repeated compilation, an overwritten Mach-O executable sometimes exits with SIGKILL before its test harness starts. A byte-identical copy to a new path executes successfully. Validation therefore executes a fresh copy of each rebuilt macOS artifact. The underlying cause has not been established.

## Array constructor index width on x86

**Status:** Resolved in x86 native validation.

The native ARRAY_INIT_INDEX lowering stored a 64-bit internal counter into the pointer-sized result local. On x86 this wrote past a 32-bit local and allowed LLVM to misoptimize array copies into repetitions of the first element. This corrupted the unwinder register context between frame walks. Lowering now converts the counter to the result type before storing it; a DUAL_TEST checks copy construction and assignment of 256 distinct pointer-sized values.

## ELF program-header count grows with exception tables

**Status:** Resolved for ARM64 native exception execution.

Alternating code and exception-table sections created a load segment for almost every input section. The broad ARM64 exception executable had 1,396 program headers and Linux rejected it with an execution-format error. Grouping non-stepping sections by permissions bounds the number of load segments while retaining each input section's relocation mapping. The rebuilt artifact loads and passes all 24 non-threaded exception cases.

## s390x canonical frame address

**Status:** Resolved; all 26 s390x native exception cases pass.

The s390x ABI defines the DWARF canonical frame address 160 bytes above the incoming stack pointer. Register recovery now subtracts that ABI save area when reconstructing the caller stack pointer.

## s390x landing-pad branch register

**Status:** Resolved; all 26 s390x native cases and 3 allocation-failure cases pass.

Using `BR %r0` suppressed the final branch after restoring the stack, so execution fell through into unrelated code. The restoration procedure now moves the destination to `%r1` and branches through it. Earlier direct propagation results alone did not expose this defect.

## Native frame lookup cost

**Status:** Performance limitation.

The runtime scans the executable's retained `.eh_frame` records for each recovered frame. Lookup cost is linear in the number of frame descriptions, multiplied by stack depth. ELF output contains `.eh_frame_hdr`, but the current runtime does not use its search table. A future shared linker-generated index could provide bounded binary search on all native formats without an external library or mutable global cache.

## Foreign frames and external unwinder interoperability

**Status:** Deliberately outside the current runtime contract.

Frame discovery covers the Qxc-produced executable. It does not discover unwind tables in external shared libraries or translate C++/SEH exception objects. Exceptions must be handled before crossing such boundaries. External unwinder integration is deferred until needed for an explicit compatibility layer; no libunwind or libgcc dependency is present.

## Compiler-generated polymorphic member and layout

**Status:** Resolved in standalone constexpr and Mac/Linux native validation.

Moving POLYMORPHIC_BASE to a compiler-owned symbol exposed an unsupported THIS AST representation and a class-placement path that admitted only enum built-ins. The generated FINAL_TYPE() body now uses the same THIS value-keyword expression as parsed source, and built-in symbols use the ordinary class-kind layout dispatch. FINAL_TYPE is registered in the parser's special member-name set. A standalone bundle without MODULE(RUNTIME) verifies type reporting, canonical views, dynamic casts and separation from lowercase user methods.
