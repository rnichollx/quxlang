(ai generated, not yet human reviewed)
# Broken-test summary

Snapshot: 2026-09-10. This summarizes the current `.qxs` skip tags and verified
findings in the [correctness-testing audit](../doc/testing-audit.md). The fixtures
are the source of truth; counts below describe declarations, not distinct bugs.

## Counts and interpretation

- **57 unconditional KNOWN_BROKEN declarations:** 48 DUAL_TEST, 6 UNIT_TEST and
  3 STATIC_TEST declarations tagged KNOWN_BROKEN.
- **1 compiled KNOWN_FAILING regression:** 1 STATIC_TEST.
  Its body and dependencies must compile; static execution is skipped.
- **199 conditionally tagged declarations:** skipped only when their
  KNOWN_BROKEN_IF condition is true and they are otherwise included.
- **9 skip-tag self-test declarations are excluded** from those counts. They
  deliberately use true/false conditions or failing bodies to test the harness;
  they are not language regressions. The 9 declarations in
  `main_test_213_known_failing.qxs` likewise exercise the KNOWN_FAILING harness
  and are excluded from regression counts.
- Latest macOS verification: **1,134 static tests passed, 56 known broken, 4 known failing**;
  **864 unit tests passed, 59 known broken, 4 known failing**. All eight configured
  targets compiled successfully. Only macOS artifacts were executed.

A known-broken body is not compiled or executed. A known-failing body and its
required dependencies must compile. DUAL_TEST contributes to both static and
runtime suites, and declaration inclusion and target conditions affect totals.
The runner totals include intentional skip-tag self-tests, so they are not bug
counts and do not equal the declaration inventory above. An unconditionally
skipped DUAL_TEST may have a passing native body and a failing constexpr body.

The macOS verification logs are `tmp/virtual-generated-validation-2.log` and
`tmp/virtual-generated-validation-run-2.log`. The validated input bundle is
byte-identical to the current testbundle. The remaining target compilation log is
`tmp/virtual-generated-final-targets.log`.
These ignored local files are evidence from the audit run, not permanent
repository artifacts. The JVM target currently
sets `run_static_tests: false` in the testbundle manifest.

## Withdrawn expectations

The 16 dedicated provenance-operation tests in fixtures 136–141 were removed.
Their expectations about region operations and address laundering were not
established and are withdrawn; they are excluded from both coverage claims and
known-broken counts.

## Recently resolved

Virtual polymorphic classes now receive implicit full-object and subobject
constructors through the same eligibility checks as ordinary classes. User
constructors remain authoritative, and generated assignment, swap, and
by-value argument construction work with the split constructor entries.
`copied_virtual_root` passes both modes. The new
`main_test_214_virtual_generated_members.qxs` covers owned virtual-base storage,
copying, moving, assignment, swap, constructor suppression, and cleanup.
Native destruction also preserves the enclosing base context instead of
installing standalone metadata into a base subobject. Polymorphic fields retain
their own complete-object metadata, including during constructor failure.
These inheritance tests remain conditionally skipped on layoutless targets.

Signed addition and compound addition across zero now pass in constexpr and
native execution. Sign-magnitude construction clears the sign of zero results,
preventing negative zero from entering fixed-width conversion. The retained
coverage also checks zero products, quotients and subtraction across I5 to I128.

Rotation counts are defined modulo BITS(T). LLVM uses the original count when
it is already in range and branches to remainder calculation only when it is
out of range, before narrowing to the operand type. Constexpr evaluation uses
the same conditional reduction. The earlier undefined-count expectations were
removed. Coverage includes every I5/U5 pattern, width boundaries, large counts,
maximum SZ counts and signed/unsigned 7-, 12- and 17-bit high-bit cases.

## Unconditional regressions

Each row lists every unconditional regression in that fixture. The observation
column distinguishes demonstrated failures from unimplemented paths and from
root causes that remain unresolved. Passing later assertions is not implied
when an earlier assertion or compilation step failed.

| Fixture | Skipped expectations | Observed failure or missing feature |
| --- | --- | --- |
| [main_test_12_interfaces](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_12_interfaces.qxs) | [`generic_owning_copy_test`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_12_interfaces.qxs#L244), [`generic_owning_type_order_test`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_12_interfaces.qxs#L256), [`interface_copy_and_null_swap`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_12_interfaces.qxs#L283) | Generic owning copy/type ordering abort constexpr on slot lifetimes but pass natively. Interface null/implementation swap leaves the destination null in both modes. |
| [main_test_31_inheritance](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_31_inheritance.qxs) | [`repeated_base_storage_identity`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_31_inheritance.qxs#L76) | Comparing repeated same-typed base subobject addresses aborts constexpr with pointer target not found in array; native execution passes. |
| [main_test_47_lambda_lifetimes](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_47_lambda_lifetimes.qxs) | [`returned_owned_capture`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_47_lambda_lifetimes.qxs#L59) | Returning a nested owned capture is rejected: Lambda capture source is not available: text. Native execution is not reached. |
| [main_test_50_pointer_values](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_50_pointer_values.qxs) | [`element_scaled_arithmetic`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_50_pointer_values.qxs#L34) | Element-scaled pointer arithmetic stalls during static validation; native execution passes. The precise operation causing the stall is unresolved. |
| [main_test_52_array_lifetimes](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_52_array_lifetimes.qxs) | [`nested_array_partial_cleanup`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_52_array_lifetimes.qxs#L132) | Nested-array element initialization loses the constructor-set complete flag in constexpr before the intended copy/unwind checks; native execution passes. |
| [main_test_53_serialization_boundaries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_53_serialization_boundaries.qxs) | [`nonbyte_signed_and_unsigned_storage`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_53_serialization_boundaries.qxs#L85) | Signed I12 serialization produces FF 0F natively instead of the expected sign-extended FF FF. Constexpr produces FF FF. |
| [main_test_54_conversion_boundaries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_54_conversion_boundaries.qxs) | [`fractional_literal_subtraction`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_54_conversion_boundaries.qxs#L92) | Subtraction of fractional literals aborts compilation with not an integer literal: 0.0. Typed floating-point subtraction has passing coverage. |
| [main_test_64_global_identity](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_64_global_identity.qxs) | [`value_argument_storage`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_64_global_identity.qxs#L53) | Mutation of value-keyed template global storage is rejected as read-only in constexpr; native identity and mutation checks pass. |
| [main_test_68_template_bindings](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_68_template_bindings.qxs) | [`nested_value_bindings`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_68_template_bindings.qxs#L37) | A value-template NAMESPACE instantiation aborts as an unsupported symboid kind. The STRUCT-wrapped control passes. |
| [main_test_81_loop_clause_lifetimes](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_81_loop_clause_lifetimes.qxs) | [`initialization_scope`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_81_loop_clause_lifetimes.qxs#L6) | INIT/EVAL locals survive the zero-iteration loop scope in both modes. The first failure prevents the later BREAK assertion from being verified. |
| [main_test_85_owned_snapshots](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_85_owned_snapshots.qxs) | [`string_versions`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_85_owned_snapshots.qxs#L6), [`array_string_versions`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_85_owned_snapshots.qxs#L22) | Owned string snapshots fail compilation: the array case cannot anchor a pointer target; the scalar string case is not an antestatal value. Native execution is not reached. |
| [main_test_96_delegate_temporaries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_96_delegate_temporaries.qxs) | [`normal_cleanup`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_96_delegate_temporaries.qxs#L38), [`failed_delegate_cleanup`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_96_delegate_temporaries.qxs#L53) | Delegate argument temporaries survive into the next delegate. Both fail constexpr; the failing-delegate case also fails natively. Normal native cleanup was not independently verified. |
| [main_test_104_arithmetic_boundaries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_104_arithmetic_boundaries.qxs) | [`signed_minimum_remainder`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_104_arithmetic_boundaries.qxs#L83) | Rechecked after the negative-zero fix: the exact body passes constexpr and native macOS for I5/I8/I12/I16/I32/I64. The source KNOWN_BROKEN tag is stale; this is no longer a reproduced failure. Remainder reconstruction reaches the same zero-sign normalization fixed for signed addition. |
| [main_test_105_exponentiation](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_105_exponentiation.qxs) | [`integer_powers`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_105_exponentiation.qxs#L32) | Built-in integer exponentiation has no matching OPERATOR^ or OPERATOR^RHS. Custom exponentiation has passing coverage. |
| [main_test_110_wide_flagsets](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_110_wide_flagsets.qxs) | [`nonbyte_word_boundary`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_110_wide_flagsets.qxs#L89), [`full_double_word`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_110_wide_flagsets.qxs#L95) | Flagset metadata is limited to 64-bit masks. The 65-bit case is rejected; the 128-bit mask also exposes an uncaught out_of_range exception. |
| [main_test_111_generated_declarations](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_111_generated_declarations.qxs) | [`snapshot_array_extent`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_111_generated_declarations.qxs#L36) | SNAPSHOT of a visible local static cannot supply a generated array extent; both static and unit-only compilation reject it. |
| [main_test_115_option_selection](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_115_option_selection.qxs) | [`expression_option_default`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_115_option_selection.qxs#L44) | An expression-valued option default is rejected because default handling accepts literal nodes rather than evaluating the expression. |
| [main_test_116_nested_serialization](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_116_nested_serialization.qxs) | [`exact_bytes_and_independent_roundtrip`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_116_nested_serialization.qxs#L17), [`array_element_bytes`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_116_nested_serialization.qxs#L89) | Fixed-array serialization returns the wrong output position in both modes. The combined aggregate case raises a constexpr array-bounds error; a shared cause has not been proven. |
| [main_test_118_atomic_pointers](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_118_atomic_pointers.qxs) | [`comparison_modes`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_118_atomic_pointers.qxs#L37) | Atomic pointer storage is rejected by atomic built-in discovery. The pointer STORE/compare-exchange assertions have not executed. |
| [main_test_119_aggregate_procedures](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_119_aggregate_procedures.qxs) | [`mixed_record_value_roundtrip`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_119_aggregate_procedures.qxs#L27) | Mixed-record indirect calls fail constexpr with initializing element out of bounds of array; the same body passes natively. |
| [main_test_128_copy_modifiers](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_128_copy_modifiers.qxs) | [`implicit_copy_rejected`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_128_copy_modifiers.qxs#L37) | NO_IMPLICIT_COPY unexpectedly allows generated copying. This is a negative semantic test whose expected rejection is missing. |
| [main_test_134_macos_aggregate_returns](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_134_macos_aggregate_returns.qxs) | [`packed_integer_return`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_134_macos_aggregate_returns.qxs#L25), [`two_register_return`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_134_macos_aggregate_returns.qxs#L42) | Native macOS C aggregate returns fail quotient checks for eight- and sixteen-byte records. The external ABI path supplies a return slot argument instead of the required register return. |
| [main_test_135_integer_type_queries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_135_integer_type_queries.qxs) | [`nonsigned_types`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_135_integer_type_queries.qxs#L45) | IS_SIGNED rejects nonintegral types rather than returning false under the draft predicate contract. The first failing type is BOOL. |
| [main_test_143_enabled_templates](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_143_enabled_templates.qxs) | [`value_condition_and_return_type`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_143_enabled_templates.qxs#L16) | Value-template overloads with mutually exclusive ENABLE_IF conditions are ambiguous before condition-based selection. |
| [main_test_144_literal_types](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_144_literal_types.qxs) | [`numeric_capture_values`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_144_literal_types.qxs#L39), [`string_capture_values`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_144_literal_types.qxs#L47) | NUMERIC_LITERAL_ANY and STRING_LITERAL_ANY argument capture are unimplemented. Exact literal-type arguments have passing controls. |
| [main_test_148_loop_limits](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_148_loop_limits.qxs) | [`exclusive_limit_and_overshoot`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_148_loop_limits.qxs#L10), [`header_order_and_frozen_limit`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_148_loop_limits.qxs#L23) | Explicit BY with LIMIT aborts constexpr with transition slots not alive. Both retained bodies pass native execution. |
| [main_test_153_inline_assembly](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_153_inline_assembly.qxs) | [`register_binding_and_memory`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_153_inline_assembly.qxs#L22) | Inline assembly has no callable overload candidates; backend lowering is also unimplemented. The register/memory assertions have not executed. |
| [main_test_157_overload_selectors](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_157_overload_selectors.qxs) | [`selected_addresses`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_157_overload_selectors.qxs#L13), [`empty_selection`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_157_overload_selectors.qxs#L24) | Explicit overload-selector expressions cannot be called or addressed through the current expression lowering. The two retained expectations remain unexecuted; the explicit indexed-call expectation was removed because that syntax is not supported in user code. |
| [main_test_158_paired_templates](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_158_paired_templates.qxs) | [`member_calls`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_158_paired_templates.qxs#L58) | The paired member-template call rejects its implicit @THIS argument. Free-function paired calls have passing coverage. |
| [main_test_159_member_template_binding](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_159_member_template_binding.qxs) | [`explicit_named_type`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_159_member_template_binding.qxs#L18) | Explicit named member-template invocation rejects implicit @THIS. The deduced member-template control passes. |
| [main_test_165_return_unequal_temporaries](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_165_return_unequal_temporaries.qxs) | [`equal_path`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_165_return_unequal_temporaries.qxs#L53) | The equal RETURN_UNEQUAL path leaves operand temporaries alive at the next statement in both modes. Unequal and throwing paths pass separately. |
| [main_test_169_match_guard_lifetimes](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_169_match_guard_lifetimes.qxs) | [`guard_continuation`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_169_match_guard_lifetimes.qxs#L6) | MATCH guard temporaries remain alive when evaluation proceeds to the selected arm. The alive-count assertion fails in both modes. |
| [main_test_174_explicit_storage_calls](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_174_explicit_storage_calls.qxs) | [`named_destruction`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_174_explicit_storage_calls.qxs#L43) | Explicit named DESTROY arguments cannot bind the generated DESTROY receiver to the user destructor, while the generated candidate rejects the named argument. |
| [main_test_179_move_assignment_spelling](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_179_move_assignment_spelling.qxs) | [`scalar_value`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_179_move_assignment_spelling.qxs#L6), [`owned_value`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_179_move_assignment_spelling.qxs#L16) | Scalar and string move-assignment spelling :< finds no OPERATOR:< or OPERATOR:<RHS. Explicit user-defined operators pass separate tests. |
| [main_test_189_nominal_templates](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_189_nominal_templates.qxs) | [`enum_instantiations`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_189_nominal_templates.qxs#L19), [`flagset_instantiations`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_189_nominal_templates.qxs#L32) | Dependent enum values cannot resolve the maximum, and dependent flagset declarations cannot resolve the width. Fixed-value type-parameter enum instantiation passes. |
| [main_test_192_array_member_initialization](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_192_array_member_initialization.qxs) | [`default_array_member`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_192_array_member_initialization.qxs#L13) | Default construction of an array member followed by a pointer fails constexpr with an array-bounds error. The exact body passes natively. |
| [main_test_197_array_member_extents](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs) | [`first_1`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L53), [`first_2`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L59), [`first_6`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L65), [`middle_1`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L77), [`middle_2`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L83), [`middle_6`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_197_array_member_extents.qxs#L89) | Nonempty array members of extents 1, 2 and 6 fail constexpr in both tested field positions. All six pass natively; both zero-element controls pass both modes. |
| [test_initguards](../quxlang/tests/testdata/testbundle/modules/runtime/sources/test_initguards.qxs) | [`abort_retry_and_complete`](../quxlang/tests/testdata/testbundle/modules/runtime/sources/test_initguards.qxs#L6), [`independent_guards`](../quxlang/tests/testdata/testbundle/modules/runtime/sources/test_initguards.qxs#L22) | Local INITGUARD construction aborts with bad_optional_access before acquisition/state assertions can run. The exact failing optional access remains unlocalized. |
| [main_test_202_nonstatic_values](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_202_nonstatic_values.qxs) | [`static_local_rejected`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_202_nonstatic_values.qxs#L45), [`static_initializer_rejected`](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_202_nonstatic_values.qxs#L52) | Function-local STATIC declarations of directly tagged NONSTATIC types compile and execute instead of being rejected, with both implicit and explicit initialization. STATIC eligibility of containing records remains unspecified and has no retained expectation. |

## Compiled known failures

| Fixture | Expectations | Observed behavior |
| --- | --- | --- |
| [test_selection_tests](../quxlang/tests/testdata/testbundle/modules/runtime/sources/test_selection_tests.qxs) | `option_boundaries_constexpr` | The constant-text option matcher compiles, but constexpr interpretation reports `unordered_map::at: key not found`. The identical option-boundary checks pass as a native UNIT_TEST. The interpreter failure has not been localized. |

## Selecting generated tests

The generated test executable accepts these options:

```sh
./tests --list-tests
./tests --list-tests --test-filter='known_failing_tests'
./tests --test-filter='*known_failing_tests::opt_in_pass' --include-failing-tests
```

Plain filters match case-sensitive substrings. Filters containing `*` or `?`
match the whole qualified name; `*` matches any sequence and `?` matches one
Unicode scalar. An empty filter selects every test. Repeated filters use the
last value. List mode includes status labels and never invokes a test body,
including when combined with `--include-failing-tests`.

KNOWN_BROKEN remains skipped even with opt-in enabled. Both status tags may be
combined, in either order; a true KNOWN_BROKEN condition takes precedence over
KNOWN_FAILING. A selected known-failing
body that fails returns a nonzero exit status under the existing fail-fast
runner. No matching tests returns 1; an unknown option returns 2. `--help`
describes the options. These options belong to generated executables, not qxc.
STATIC_TEST KNOWN_FAILING bodies compile but are not executed by the compiler.

## Conditional backend and platform gaps

These counts come from declared conditions, before evaluating INCLUDE_IF and
module selection. They are not execution results for Linux, Windows or JVM.
Temporary implementation gaps use KNOWN_BROKEN_IF so their expectations remain
visible. Permanent native-layout restrictions, such as integer-pointer
conversions on layoutless targets, can still use INCLUDE_IF.

| Condition | Declarations | Scope |
| --- | ---: | --- |
| `ARCH_IS_LAYOUTLESS` | 159 | Exception handling and paths that reach throwing constructors; inheritance/polymorphism; selected atomic and unevaluated-expression paths. Representative fixtures: [main_test_37_exceptions](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_37_exceptions.qxs), [main_test_31_inheritance](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_31_inheritance.qxs), [main_test_61_atomic_values](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_61_atomic_values.qxs), [main_test_186_unevaluated_owned_expressions](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_186_unevaluated_owned_expressions.qxs). The exception-allocation SIZEOF failure is a missing layoutless exception path, not a reason to disable SIZEOF for native targets. |
| `ARCH_IS_JVM` | 9 | Atomic thread/publication/ordering tests, owned TLS tests and an inheritance case. Fixtures: [main_test_62_atomic_threads](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_62_atomic_threads.qxs), [main_test_63_thread_local_objects](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_63_thread_local_objects.qxs), [main_test_131_atomic_publication](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_131_atomic_publication.qxs), [main_test_132_atomic_total_order](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_132_atomic_total_order.qxs), [main_test_31_inheritance](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_31_inheritance.qxs). |
| `ARCH_IS_X86 \|\| ARCH_IS_LAYOUTLESS` | 1 | The 64-bit atomic read-modify-write case in [main_test_61_atomic_values](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_61_atomic_values.qxs); x86 lacks non-native atomic lowering and Cortado lacks the required operations. Narrower native cases remain active. |
| `ARCH_IS_X86` | 1 | The 64-bit rotation-cycle oracle in [main_test_70_rotation_counts](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_70_rotation_counts.qxs) reaches missing `__udivdi3` runtime support. Narrower cycle cases remain active. |
| `BACKEND_LLVM == FALSE` | 1 | Half-precision arithmetic in [main_test_06_arguments_and_operators](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_06_arguments_and_operators.qxs). |
| None of Linux, Windows or macOS | 18 | Thread/concurrency support and cross-thread exception ownership in [main_test_22_threads](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_22_threads.qxs), [main_test_25_concurrency](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_25_concurrency.qxs) and [main_test_38_exception_threads](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_38_exception_threads.qxs). The source uses three equivalent orderings of this OS condition. |

## Known gaps outside these skip counts

- Native CHECKED integer conversion does not enforce its range contract. The
  [checked-conversion fixtures](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_69_checked_conversions.qxs)
  contain passing STATIC_TEST EXPECT_FAIL expectations for invalid inputs, but
  those static passes do not validate native trapping. The ordinary unit-test
  runner cannot treat process termination as a passing result.
- [Assertion-message tests](../quxlang/tests/testdata/testbundle/modules/tests/sources/main_test_163_assertion_messages.qxs)
  verify successful condition evaluation. Separate native failure entrypoints
  showed that ASSERT_FAIL does not render the supplied message tag. The passing
  tests do not establish message rendering.
- Source expressions using TARGET and nonempty CSTRING_CONSTANT/DATA_CONSTANT
  construction still lack a sufficiently established public contract for
  positive expectations. See the audit's unsupported/pending entries; these
  are not included in the tagged regression counts.

## Maintaining this summary

When resolving an issue, run its retained body in the affected execution mode
before removing the tag. Recheck the ordinary macOS suite and compile the full
bundle for every configured target. Record native versus constexpr differences
explicitly, and update the source-tag counts when declarations change. Preserve
intentional skip-tag self-tests independently of regression cleanup.
