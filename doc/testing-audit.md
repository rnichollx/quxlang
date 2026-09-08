(ai generated, not yet human reviewed)
# Quxlang correctness-testing audit

This audit tracks executable correctness coverage for the language features
accepted by the current compiler. Tests belong in the `.qxs` files under
`quxlang/tests/testdata/testbundle/`. Syntax-error testing is deferred.

Validation uses the compiler's static evaluator and native macOS ARM64
executables. Linux, Windows, and JVM execution are outside this audit's current
validation scope. A feature's appearance in a fixture is not evidence that all
of its cases have been reviewed.

A grouped inventory of skipped regressions and backend gaps is maintained in
[Broken-test summary](../docs/broken-test-summary.md).

## Status definitions

- **Reviewed:** implementation and assertions inspected; the listed coverage
  passes static evaluation and/or native execution as stated.
- **In progress:** new assertions exist but validation or coverage review remains.
- **Pending:** candidate fixtures identified; completeness has not been assessed.
- **Unsupported:** the current implementation cannot execute the feature; no passing correctness coverage is claimed.

## Feature inventory

Fixture names below are relative to `modules/tests/sources/` in the test bundle.

| Area | Status | Existing or added coverage | Remaining work |
| --- | --- | --- | --- |
| Short-circuit `&&`, `||`, `&!`, `|!`, `^>`, `^<` | Reviewed | `main_test_41_short_circuit.qxs`: all truth-table entries, skipped operands, lifetime boundaries, reverse destruction, nesting, exceptions, loops, returns, enclosing temporaries | No known gap in the reviewed behavior |
| String and byte literals; decimal and exact fractional values | Reviewed | `main_test_42_literal_values.qxs`: all supported escape bytes, UTF-8 bytes, embedded zeros, integer boundaries, exact fractional arithmetic | `main_test_162_decimal_spellings.qxs` adds zero-prefixed decimal integers and exact fractions with leading/trailing zeroes; both DUAL_TESTs pass. The conversion review found cross-width float constructors unimplemented; see the qualifier/conversion row |
| Runtime `CHOOSE` expression | Unsupported | The AST/lowering contains expression_choose, but the current expression parser only accepts STATIC_CHOOSE; runtime lowering also remains unimplemented | No runtime CHOOSE syntax fixture is added because syntax-error tests are excluded |
| Eager Boolean `^^`, `^!` | Reviewed | `main_test_43_scalar_operations.qxs`: all truth-table entries with observable left-to-right single evaluation of both operands | No known gap in the reviewed behavior |
| Arithmetic and compound assignment | Reviewed | `main_test_43_scalar_operations.qxs`: positive/negative signed division, signed remainder and compound operations at I8/I16/I32/I64; indexed receivers evaluated once | Known constexpr signed-addition failure at zero crossing; `main_test_87_member_arithmetic.qxs` covers normal/RHS member dispatch, written evaluation order and compound receiver mutation; `main_test_88_expression_grouping.qxs` covers arithmetic precedence, associativity and parenthesized overrides; `main_test_104_arithmetic_boundaries.qxs` covers representable signed extrema and unsigned maxima across six widths; signed minimum remainder fails constexpr and is KNOWN_BROKEN; `main_test_105_exponentiation.qxs` covers custom exponentiation dispatch/grouping, while built-in I32 exponentiation is unimplemented and retained as KNOWN_BROKEN; listed arithmetic cases reviewed |
| Bitwise operations, shifts, rotations | Reviewed | `main_test_43_scalar_operations.qxs`: unsigned truth functions and assignment forms; `main_test_70_rotation_counts.qxs`: cycles and large counts; `main_test_71_logical_shifts.qxs`: valid shift counts and signed zero-fill; `main_test_88_expression_grouping.qxs`: bitwise precedence; `main_test_103_signed_bitwise.qxs`: negative operands across six signed widths | Non-byte large-rotation regression remains KNOWN_BROKEN; no remaining listed coverage gap |
| Comparisons, ordering, floating-point special values | Reviewed | `main_test_59_comparison_boundaries.qxs`: all 25 ordered operand pairs and seven comparison operators for six signed widths, six unsigned widths and F32/F64 finite values; signed minima and unsigned high-bit maxima included | `main_test_60_float_special_values.qxs` now covers signed zero, infinities and NaN operand symmetry in F32/F64; `main_test_89_float_subnormals.qxs` covers gradual underflow, normal/subnormal arithmetic and signed-zero rounding; `main_test_90_member_comparisons.qxs` covers all derived comparisons from a custom ordering with invocation counts and operand evaluation; `main_test_108_nan_encodings.qxs` covers eight signed signaling/quiet NaN encodings per width, canonical serialization and every ordered pair after deserialization; `main_test_72_enum_ordering.qxs` covers explicit enum representations and all four-bit ALLOW_UNKNOWN pairs; `main_test_109_pointer_ordering.qxs` covers all 25 within-array ordered pairs including one-past endpoints for three element widths; no remaining listed comparison gap |
| Assignment, move, swap, increment/decrement | Reviewed | `main_test_49_mutation_values.qxs`: postfix results across eight integer widths, non-byte unsigned wraparound, indexed assignment/swap/incdec evaluation order, scalar aliases, generated owned-record copy and swap | Owned moves are covered in struct/return fixtures; `main_test_91_member_incdec.qxs` covers custom postfix result contracts, once-only receivers and combined expression order; `main_test_92_pointer_incdec.qxs` covers every forward/reverse postfix position and endpoint for three element widths; `main_test_93_member_assignment.qxs` covers custom assignment/swap dispatch and self-operation effects; no known gap in the reviewed mutation behavior |
| `IF`, `UNLESS`, `ELSE` | Reviewed | `main_test_44_control_flow.qxs`: mixed chains, evaluation order, skipped conditions and selected branch cleanup; `main_test_78_conditional_lifetimes.qxs`: nested conditions, temporary cleanup before branch entry, reverse branch cleanup and throwing condition unwinding | No known gap in the reviewed conditional behavior |
| `WHILE`, `LOOP`, break/continue | Reviewed | `main_test_44_control_flow.qxs`: zero/multiple iterations, INIT/EVAL/TEST/POSTTEST/STEP ordering, filter progress, named exits and cleanup, equal/empty sequence endpoints | `main_test_80_iterator_lifetimes.qxs` covers mutable item/indexed-value aliases, filter temporary cleanup, CONTINUE advancement and explicit STEP cleanup/BREAK suppression; `main_test_81_loop_clause_lifetimes.qxs` covers POSTTEST temporary cleanup; INIT/EVAL scope cleanup fails both execution modes and is KNOWN_BROKEN; `main_test_121_loop_handler_exits.qxs` covers handler BREAK/CONTINUE cleanup, restoration of enclosing exception state and STEP ordering/suppression; `main_test_148_loop_limits.qxs` covers exclusive iterator LIMIT, overshoot, captured limits, header order and filtered CONTINUE cleanup; explicit-BY limit cases abort constexpr state transitions and are KNOWN_BROKEN, while all three pass native isolation; listed loop cases reviewed |
| Labels and `GOTO` | Reviewed | `main_test_44_control_flow.qxs`: forward/backward transfers recreate and destroy scoped locals; `main_test_79_label_lifetimes.qxs`: nested labelled breaks preserve enclosing locals, reverse deferred cleanup, GOTO from a handler restores exception state | No known gap in the reviewed label behavior |
| Return and `RETURN_UNEQUAL` | Reviewed | `main_test_44_control_flow.qxs`: operand evaluation, early return versus equal continuation, reverse cleanup; `main_test_65_procedure_calls.qxs`: reference identity and mutation; `main_test_77_return_lifetimes.qxs`: selected owned moves, copied results and throwing return construction | `main_test_165_return_unequal_temporaries.qxs` adds owned custom-comparator operands; equal-path temporary cleanup fails both modes and is KNOWN_BROKEN, while unequal-return and throwing-comparator paths pass |
| Local/static/global/thread-local declarations | Reviewed | `main_test_63_thread_local_objects.qxs`: simultaneous owned TLS objects, parent isolation, destruction before join, repeated-thread fresh initialization; existing scalar TLS cases inspected | `main_test_64_global_identity.qxs` covers type-keyed owned globals, alias identity, repeated returned references and value-keyed mutable storage; constexpr rejects mutation of the value-keyed initializer case. `main_test_112_global_initialization.qxs` covers a forward-declared dependency chain with a shared base, once-only initialization, persistent mutation, and cleanup/retry after a throwing initializer; `main_test_149_constexpr_global_tags.qxs` verifies reads, mutation and reference identity for CONSTEXPR_READABLE/CONSTEXPR_READWRITE declarations; those tags currently have no semantic consumers beyond parsing; listed initialization cases reviewed |
| Static evaluation and generation | Reviewed | `main_test_45_static_generation.qxs`: nested expansion, frozen array/scalar snapshots, static branch selection and side effects, generated block cleanup, repeated calls, STATIC_CHOOSE reference identity | `main_test_85_owned_snapshots.qxs` retains two KNOWN_BROKEN owned-string snapshot regressions; `main_test_111_generated_declarations.qxs` covers initialized generated locals, repeated calls, return/exception exits from every expansion, and a KNOWN_BROKEN SNAPSHOT-dependent array extent; phase selection is covered by fixture 67; listed static statement forms reviewed |
| Calls, named/positional arguments, defaults, packs | Reviewed | `main_test_46_call_binding.qxs`: receiver/source order versus named binding, per-call defaults and caller lookup isolation, positional defaults, grouped and empty packs, aliased reference packs, throwing argument cleanup | `main_test_94_default_lifetimes.qxs` covers owned default temporaries, failure at each factory and explicit suppression; composite forwarding is covered in `main_test_76_composite_calls.qxs`; `main_test_95_owned_packs.qxs` covers heterogeneous value packs and indexed move forwarding on normal/exceptional exit; general pack-spread syntax is not implemented in the call parser |
| Templates and overload selection | Reviewed | `main_test_68_template_bindings.qxs`: named/local-name binding, positional and mixed value arguments, nested struct bindings; four ranking cases in `main_test_26_function_selection_ranking.qxs` upgraded to DUAL_TEST | Templated namespace instantiation aborts and is KNOWN_BROKEN; `main_test_106_template_constraints.qxs` covers exact type constraints, qualifiers and aliases; two existing template-kind cases now run as DUAL_TEST, covering TYPE, CLASS and nested capture; main_test_107_method_bindings.qxs covers copied receiver bindings, subsequent receiver mutation and overload selection after binding. `main_test_143_enabled_templates.qxs` covers captured-type ENABLE_IF selection, disabled invalid bodies and no-enabled-candidate rejection; same-signature value-template overloads fail before ENABLE_IF selection and remain KNOWN_BROKEN. Arbitrary partial argument binding and parameter defaults are not supported by the current parser |
| Qualifiers, references, pointers, conversions | Reviewed | `main_test_50_pointer_values.qxs`: element-scaled offsets, one-past endpoints, signed differences, ordering, reverse traversal, reference binding and pointer-slot nullness | Known pointer constexpr stall; integer conversion modes and exact integer-to-float limits now covered by main_test_54_conversion_boundaries.qxs; `main_test_123_float_rounding.qxs` covers APPROXIMATE integer-to-float ties for both signs and 64-bit maxima; float-to-integer and cross-width float constructors are unimplemented in current built-in overloads; `main_test_124_reference_qualifiers.qxs` covers WRITE output aliasing, array-element outputs, const reference/pointer identity and stored mutable-pointer access; TEMP/MOVE/FORWARD behavior is covered by ownership and composite fixtures; listed qualifier cases reviewed |
| Arrays, storage, placement and destruction | Reviewed | `main_test_52_array_lifetimes.qxs`: copy failure at each position, reverse destruction, independent copied elements, empty arrays, nested-array initialization and partial cleanup regression | Nested-array constexpr constructor state remains KNOWN_BROKEN; `main_test_101_storage_reuse.qxs` covers repeated owned placement/destruction and retry after failed construction; `main_test_102_allocated_lifetimes.qxs` covers allocated partial construction, completed-prefix cleanup, slot reuse and deallocation; listed lifetime cases reviewed with the nested-array regression retained |
| Structures, constructors/destructors, member operators | Reviewed | `main_test_57_struct_lifetimes.qxs`: generated copy failure at every field, independent copied storage, generated move ownership, constructor-body failure cleanup, destructor body before reversed field cleanup; existing generated-destructor tests inspected | `main_test_58_constructor_delegates.qxs` now covers named delegate forwarding, reversed initializer-list order, omitted scalar initialization, failed field delegates and direct bases; argument-expression failure now also verifies that affected constructors and later delegates are skipped; `main_test_96_delegate_temporaries.qxs` retains normal and failing delegate temporary-boundary regressions as KNOWN_BROKEN; custom member arithmetic/comparisons/assignment are covered in fixtures 87/90/91/93; `main_test_126_struct_modifiers.qxs` covers explicit operations under NO_IMPLICIT_CONSTRUCTORS/NO_IMPLICIT_ASSIGNMENT/NO_DEFAULT_SWAP and rooted field identity during exception cleanup; `main_test_127_ibc_values.qxs` covers IBC_STRUCT copying, assignment, swap, indirect calls and declaration-order reflection plus macOS ARM64 layout; `main_test_128_copy_modifiers.qxs` verifies NOT_COPYABLE copy/assignment rejection and generated movement; NO_IMPLICIT_COPY is ineffective and retained as KNOWN_BROKEN. `main_test_129_final_structs.qxs` covers final value copying, assignment, movement, swap and containment, final polymorphic leaf dispatch, and semantic rejection of direct, aliased and virtual final bases |
| Namespaces, imports, aliases, visibility | Reviewed | `main_test_33_aliases.qxs`: 16 existing positive alias tests upgraded to DUAL_TEST; two added tests cover shared aliased-array storage and named primary argument binding; includes cross-module, lexical and permitted private-target cases | `main_test_113_import_lookup.qxs` covers repeated renamed imports, cross-module type/global identity, permitted private-member access through a public function and declaration-site lexical lookup; fixture 114 verifies module-wide import scope across source files. Existing aliases/public-field fixtures cover CLASS, MODULE and named privacy scopes; `main_test_156_privacy_scope_lists.qxs` adds union-of-scopes permissions, nested descendants, mixed CLASS/friend member access, and rejection from unlisted scopes; all six cases pass and listed lookup forms are reviewed |
| Conditional declarations and options | Reviewed | `main_test_24_include_if.qxs`: Boolean/numeric option defaults, inactive overload exclusion, template-dependent fields/aliases/methods, size and alignment, conditional namespace lookup | `main_test_86_conditional_lifetimes.qxs` covers template-selected constructors/destructors and owned/scalar fields, reflection exclusion and destruction ordering; `main_test_115_option_selection.qxs` covers configured numeric dependencies, aliases/array extents, Boolean declaration selection and string-option construction; a temporary bundle verifies exact Boolean/string overrides. Expression-valued defaults are retained as KNOWN_BROKEN because default evaluation currently accepts literals only; listed option forms reviewed |
| Lambdas and captures | Reviewed | `main_test_47_lambda_lifetimes.qxs`: mixed capture modes, independent copied value state, shared reference state, owned capture cleanup, nested capture modes; escaping local capture regression | Nested-local capture rejection remains KNOWN_BROKEN; moved closures are covered by `main_test_83_defer_captures.qxs`; `main_test_97_closure_copy_failures.qxs` covers failure at every owned capture during initial construction and closure copying; `main_test_98_lambda_signatures.qxs` covers renamed/defaulted parameters, NOEXCEPT and reference returns into borrowed/owned capture storage; parser capture forms reviewed, with the escaping-capture failure retained |
| Interfaces, generics and implementations | Reviewed | `main_test_12_interfaces.qxs`: three owning-generic cases upgraded to DUAL_TEST; new copied reference rebinding and interface null/implementation swap tests | Generic comparison constexpr lifetime failures and interface swap failure on both paths remain KNOWN_BROKEN; `main_test_122_generic_moves.qxs` covers concrete copy/destruction counts, owning move transfer, independent owning copies and reference move/rebinding; listed interface/generic cases reviewed |
| Enum and flagset declarations | Reviewed | `main_test_48_enum_flagset_values.qxs`: implicit allocation around reservations, all byte inputs for four-bit ALLOW_UNKNOWN decoding, composite membership, all 64 three-bit operand pairs for eight bitwise operators and assignment forms | `main_test_110_wide_flagsets.qxs` covers 63/64-bit high masks, implicit allocation past reserved bits, unnamed-bit preservation, all binary bitwise operators and guarded serialization; 65/128-bit expectations are KNOWN_BROKEN because flagset metadata is limited to 64 bits. Existing enum fixtures cover wide explicit/implicit values, NULL/default cases and reserved values; listed declaration forms reviewed |
| Union/variant, `MATCH`, `VISIT` | Reviewed | `main_test_55_variant_selection.qxs`: every nested VISIT alternative pair, mutable bindings, guards, tag changes, branch cleanup. `main_test_73_union_selection.qxs`: boxed/inline named alternatives sharing a payload type, independent copies, repeated assignment and reset, named guards and both fallbacks. `main_test_74_union_lifetimes.qxs`: owned payload replacement, match unwinding and failed copy construction in both representations | `main_test_99_nested_unions.qxs` covers all four boxed/inline nesting combinations, moved inner ownership, independent outer copies and replacement cleanup; `main_test_100_union_assignment_failures.qxs` verifies failed argument-copy preservation and successful retry in both representations; no known gap in the listed union review cases |
| Inheritance and polymorphism | Reviewed | `main_test_31_inheritance.qxs`: copied derived dispatch and independent base storage; repeated base selectors and copying; three constructor/type-trait cases upgraded to DUAL_TEST | Repeated-base pointer comparison constexpr abort remains KNOWN_BROKEN; `main_test_125_virtual_base_casts.qxs` covers shared-root identity, branch/complete-object downcasts, failed casts preserving state, and null/const casts. Complete-object virtual-diamond copying has no copy constructor overload and is retained as KNOWN_BROKEN; `main_test_142_final_virtual_methods.qxs` covers VIRTUAL(FINAL), VIRTUAL(OVERRIDE, FINAL), inherited dispatch, distinct same-named overloads, copied leaf storage and rejection of further overrides; listed inheritance cases reviewed |
| Composites and reflection expressions | Reviewed | `main_test_34_composites.qxs`, `main_test_35_public_fields.qxs`: 16 positive cases upgraded to DUAL_TEST; new mixed-copy storage and empty split/join checks; public reflection privacy, direct declaration ordering, qualifiers, aliases, WRITE projection and single evaluation | `main_test_75_reflection_lifetimes.qxs` covers owned temporary lifetime across normal and throwing calls, reverse cleanup of unselected fields and ownership transfer through a reflected move; `main_test_76_composite_calls.qxs` covers APPLY reference results across split/join and owned parameter forwarding with normal/exception cleanup; `main_test_120_heterogeneous_composites.qxs` covers strings, borrowed arrays and scalar arguments through split/join/APPLY, field-type metadata and storage identity, plus independent selected owned fields alongside shared borrowed fields; listed composite/reflection cases reviewed |
| Serialization and stringlike conversions | Reviewed | `main_test_53_serialization_boundaries.qxs`: exact LEB128/UINTANY boundaries, maximum U64 LEB128, returned iterators and adjacent guards, signed/unsigned 12-bit serialization | Native signed-padding discrepancy; `main_test_116_nested_serialization.qxs` covers exact nested-field and U128 bytes with independent reconstruction, guards and iterators. Fixed-array output-position and combined aggregate regressions remain KNOWN_BROKEN; `main_test_117_stringlike_bytes.qxs` covers a 130-byte STRINGLIKE payload with a multi-byte length prefix, embedded zeros, UTF-8 bytes, exact iteration and independent owned copies; listed serialization/stringlike cases reviewed |
| `DEFER` and exception handling | Reviewed | `main_test_36_defer.qxs`, `main_test_37_exceptions.qxs`: deferred action forms, typed/default/sentinel handlers and rethrow; `main_test_51_handler_lifetimes.qxs`: handler exits and retained payloads; `main_test_82_nested_exceptions.qxs`: replacement and restoration; `main_test_83_defer_captures.qxs`: capture ownership and construction failure; `main_test_84_handler_selection.qxs`: nominal dispatch and propagation through nonmatching handlers | No known gap in the reviewed defer and exception behavior; partial-construction regressions are tracked under their aggregate features |
| `RUNTIME`, assertions, panic and explicit compilation failure | Reviewed | `main_test_67_runtime_selection.qxs`: nested CONSTEXPR/NATIVE selection, static selection inside runtime branches, excluded assertion side effects, phase-specific returns and cleanup; existing lowering-failure contracts inspected | `main_test_130_phase_dependencies.qxs` verifies phase-filtered direct and indirect callee dependencies, shared continuations disconnected by phase-specific returns, and rejection of invalid lowering dependencies inside ordinary IF(FALSE); standalone macOS ARM64 executions verify exit code 1 and diagnostics for explicit/default PANIC, ASSERT(FALSE), MATCH DEFAULT FAIL, valueless MATCH and invalid/valueless UNWRAP; syntax-error checks excluded |
| Procedures, assembly and external calls | Reviewed | `main_test_65_procedure_calls.qxs`: selected procedure pointers, named argument evaluation, reference parameters/returns and indirect exception cleanup; existing positional static procedure call inspected | Positional pointer-array calls, replacement, independent copies and swap are now covered in the same fixture; `main_test_66_arm64_calls.qxs` covers eight register plus two full-width stack arguments and pointer mutation/return; mixed F32/I32 and F64/I64 calls plus nine-double register/stack arguments are now covered; `main_test_119_aggregate_procedures.qxs` covers homogeneous floating-point and mixed scalar/array aggregate arguments and returns through procedure pointers; mixed records pass natively but fail constexpr with an array-bounds error and remain KNOWN_BROKEN; `main_test_133_macos_external_calls.qxs` covers actual libSystem floating/integer arguments, floating returns, integer/floating output pointers, and overlapping byte-copy arguments/results; `main_test_134_macos_aggregate_returns.qxs` retains native div_t/lldiv_t aggregate-return failures as KNOWN_BROKEN; the listed call forms have been reviewed |
| Atomics and concurrency expressions | Reviewed | `main_test_61_atomic_values.qxs`: fetch and void RMW operations across five modes and four integer types; CAS success/failure and expected-value writeback across nine mode pairs; Boolean CAS | `main_test_62_atomic_threads.qxs` adds release/acquire payload publication, contended fetch-add return values and CAS retry loops; `main_test_118_atomic_pointers.qxs` verifies independent atomic fields in adjacent array elements and reference-selected RMW/CAS; atomic pointer operations are unimplemented and retained as KNOWN_BROKEN; `main_test_131_atomic_publication.qxs` adds 1,000-round relaxed-RMW release-sequence relay and failed-CAS acquire publication; `main_test_132_atomic_total_order.qxs` checks the forbidden both-zero outcome of sequentially consistent store/load pairs across two locations over 1,000 synchronized rounds; listed atomic forms and ordering patterns reviewed |

Additional parser cross-check:

| Area | Status | Evidence and remaining work |
| --- | --- | --- |
| Integer type predicates | Reviewed | `main_test_135_integer_type_queries.qxs` covers six integer widths, aliases, unsigned BYTE classification, and nonintegral Boolean/float/record/pointer/reference types. IS_SIGNED on nonintegral types contradicts the draft predicate contract and is KNOWN_BROKEN |
| Allocation-region expressions | Reviewed | `main_test_136_region_resize.qxs` checks unchanged-count pointer/live-value preservation and retains repeated resize-pointer evaluation as KNOWN_BROKEN. `main_test_137_region_roundtrips.qxs` covers single/multi region end/rebegin between object lifetimes and native interior address escape/discovery. `main_test_138_dynamic_regions.qxs` covers dynamic begin/unchanged-size resize/end with once-only operand order and nested typed storage; PARENT_ALLOC_ADDRESS returns the wrong type for storage pointers and is KNOWN_BROKEN. `main_test_139_region_relocation.qxs` verifies empty-region operand order and retains failed live-integer relocation as KNOWN_BROKEN; `main_test_140_region_phase_restrictions.qxs` verifies constexpr rejection of both address-laundering expressions and retains missing single-region phase enforcement as KNOWN_BROKEN; `main_test_141_remaining_region_phases.qxs` retains missing constexpr rejection for multi/dynamic region lifecycles, parent lookup and empty relocation; the listed region operations and phase boundaries have been reviewed, with provenance instrumentation itself still unimplemented |
| Explicit literal types and captures | Reviewed | `main_test_144_literal_types.qxs` covers NUMERIC_LITERAL_TYPE and STRING_LITERAL_TYPE identity and overload selection, including embedded zeros. NUMERIC_LITERAL_ANY and STRING_LITERAL_ANY positive capture cases are unimplemented and retained separately as KNOWN_BROKEN |
| DECAY type deduction | Reviewed | `main_test_145_decay_capture.qxs` verifies named value capture, bare return deduction and explicitly qualified reference capture; `main_test_159_member_template_binding.qxs` verifies deduced member calls retain receiver identity across scalar instantiations |
| Constant-family nominal identity | Reviewed | `main_test_146_constant_type_identity.qxs` covers CSTRING_CONSTANT/DATA_CONSTANT identity, aliases and overload distinction without constructing undocumented values |
| Constant-family source construction | Pending | `list_builtin_constructors.cpp` supplies literal constructors for STRING_CONSTANT and NUMERIC_CONSTANT, but none for CSTRING_CONSTANT or DATA_CONSTANT. DATA_CONSTANT is synthesized internally for serialized static-global initialization in `co_vmir_generator2.hpp`; CSTRING_CONSTANT-specific source references are limited to parsing, naming and mangling. Public nonempty-value construction and intended invariants require a contract before positive assertions can be written; asked the user for clarification |
| Runtime test and stepping metadata | Reviewed | `main_test_147_runtime_metadata.qxs` verifies test names/known-broken flags and stepping bounds/table availability |
| Runtime initialization guard types | Reviewed | INITGUARD is parser-restricted to MODULE(RUNTIME); runtime `test_initguards.qxs` retains local-construction-blocked state tests as KNOWN_BROKEN. INITGUARD_LOCK is synthesized internally by global-accessor lowering. Completion/abort behavior is exercised through the global initialization retry fixtures; no standalone source-level lock acquisition API was identified |
| Ignored positional arguments | Reviewed | `main_test_150_ignored_arguments.qxs` covers interleaved unnamed positions, argument evaluation order, declaration-context defaults, and empty/heterogeneous owned ignored packs through normal and exceptional exits; all three DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| Inherited option defaults | Reviewed | `main_test_151_inherited_options.qxs` covers forward/chained numeric defaults, inherited target overrides, declaration-context lookup, Boolean selection, embedded-zero/UTF-8 strings, and independent derived overrides. Semantic rejection cases cover direct/indirect cycles, kind mismatch, non-option/missing sources, and missing terminal values; both default and temporary derived-override configurations pass static evaluation and native macOS ARM64 execution |
| DEFAULTED argument metadata | Reviewed | `parse_argif` is the only parser that consumes DEFAULTED, and a repository-wide source search finds only its definition and forward declaration, with no callers. It is not reachable source syntax in the current parser; no syntax-error fixture was added |
| Optional external procedures | Reviewed | `main_test_152_optional_externals.qxs` verifies present libSystem direct calls with default CCALL, nonnull copied callable addresses, and a missing optional symbol's null address without calling it. All three native macOS tests pass; the absent symbol remains present in the emitted binary, confirming it was not simply omitted |
| External type identity | Reviewed | `main_test_154_external_type_identity.qxs` checks EXTERN_TYPE identity, alias preservation and distinction from another external declaration, a Quxlang record and an integer. No managed object is constructed. GC pointer construction and checked managed casts require JVM execution, outside current validation scope |
| Integer bit-count operands | Reviewed | `main_test_205_integer_bit_counts.qxs` checks signed/unsigned declared widths across byte and word boundaries, aliases, numeric-literal results, unevaluated TYPEOF calls and noninteger/object operand rejection |
| Layout operand semantic constraints | Reviewed | `main_test_204_layout_operand_constraints.qxs` rejects object, reference, function and namespace names in SIZEOF/ALIGNOF and verifies explicit DECLTYPE operands |
| Layout query type operands | Reviewed | `main_test_203_layout_type_operands.qxs` checks one- and two-dimensional array size/alignment identities for six scalar types, unevaluated owned construction through TYPEOF, and value-specific numeric-literal result types. Physical layout checks exclude layoutless targets |
| NONSTATIC local values and direct STATIC declarations | Reviewed | `main_test_202_nonstatic_values.qxs` checks local default/copy/move/assignment operations, mutable aliases, containing-record independence, and known-broken rejection expectations for directly tagged function-local STATIC types; STATIC eligibility of containing records remains unspecified and is not tested |
| Implicit default-constructor suppression | Reviewed | `main_test_201_default_constructor_modifier.qxs` checks NO_IMPLICIT_DEFAULT_CONSTRUCTOR with generated copy/move/assignment, user default constructors, defaulted constructor arguments and explicit member delegates. Semantic rejection cases cover missing local/expression arguments and implicit member default construction |
| Explicit floating-point type spellings | Reviewed | `main_test_200_explicit_float_types.qxs` checks F32E8/F32 and F64E11/F64 canonical identity, exponent-width distinction, arithmetic, mutable aliases and equivalent direct/indirect callable signatures. Nonstandard-format arithmetic is not claimed by the type-identity assertions |
| Custom dereference and wrapper addresses | Reviewed | `main_test_198_custom_dereference.qxs` checks mutable/constant dereference overloads, once-only compound receivers, wrapper address-taking without custom dispatch, and live external aliases after temporary wrapper destruction. `main_test_199_dereference_boundaries.qxs` checks receiver cleanup before enclosing cleanup on a throw and rejects mutation or mutable-reference binding through constant dereference results |
| Custom unary postfix operators | Reviewed | `main_test_193_custom_unary_postfix.qxs` checks independent `??`, `?!`, `!!` and `#!!` member dispatch, once-only receiver evaluation, source-value preservation, temporary destruction and chained Boolean negation. `main_test_194_unary_postfix_exceptions.qxs` checks receiver destruction before outer cleanup and handler entry when each operator throws |
| Index parameter binding and constraints | Reviewed | `main_test_195_index_parameter_binding.qxs` covers trailing defaults, explicit replacements, empty/nonempty positional tails, source evaluation order and stable address results for both bracket spellings. `main_test_196_index_constraints.qxs` rejects missing/extra coordinates, assignment through a constant result and mutable address indexing through a constant receiver |
| Multi-argument indexing | Reviewed | `main_test_190_multidimensional_indexing.qxs` checks two-coordinate `[]` and `[&]`, receiver-before-index evaluation, once-only compound assignment, mutable/constant overload selection and returned storage identity |
| Fixed-array member extents and positions | Reviewed | `main_test_197_array_member_extents.qxs` tests extents 0, 1, 2 and 6 with arrays first or between scalar fields. Both empty cases pass constexpr; all six nonempty cases independently fail with initializing element out of bounds of array and remain KNOWN_BROKEN. All eight bodies pass native macOS execution in isolation |
| Fixed-array member initialization | Reviewed | `main_test_192_array_member_initialization.qxs` isolates default construction of a record with a six-element I32 array followed by a pointer. Constexpr aborts with initializing element out of bounds of array; the positive expectation remains KNOWN_BROKEN; the identical body passes native macOS execution in isolation |
| Index argument lifetimes | Reviewed | `main_test_191_index_argument_lifetimes.qxs` checks owned first indices remain alive through either operator body, normal cleanup before continuation, and cleanup without invocation when the second index throws. The exception case uses KNOWN_BROKEN_IF for Cortado |
| Nominal templates | Reviewed | `main_test_189_nominal_templates.qxs` verifies fixed-value enum type-parameter identity and independent state. Enum values depending on template parameters fail to resolve maximum; flagset widths depending on template parameters fail to resolve width. Both dependent-value cases remain KNOWN_BROKEN; the type-parameter DUAL_TEST passes both modes |
| Nominal associated declaration bodies | Reviewed | `main_test_188_nominal_associated_declarations.qxs` checks ENUM and FLAGSET associated aliases, owning-scope enumerator lookup, an enum factory, constant methods, mutable methods and generated membership fields observed through a constant reference. Both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| IBC_ENUM values | Reviewed | `main_test_187_ibc_enum_values.qxs` verifies explicit DEFAULT, numeric ordering, nominal distinction, explicit-width serialization, implicit-width initialized values, all 256 ALLOW_UNKNOWN byte round trips, and rejection of default construction without DEFAULT. Two DUAL_TESTs and one semantic rejection test pass; foreign-ABI behavior is not claimed |
| Unevaluated owned expressions | Reviewed | `main_test_186_unevaluated_owned_expressions.qxs` verifies TYPEOF of owned construction and a throwing call has no construction, destruction or exception side effects, and TYPEOF of PLACE leaves its slot empty for a subsequent real lifetime. Both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| Active lifetimes in typed storage | Reviewed | `main_test_185_storage_active_lifetimes.qxs` adds seven STATIC_TEST EXPECT_FAIL cases for PUN before construction/after destruction, construction over a live object, empty or duplicate destruction, and PUN/destruction selecting an allowed but inactive type. These exercise constexpr lifetime enforcement; invalid native execution is not attempted |
| Procedure pointer compatibility | Reviewed | `main_test_184_procedure_compatibility.qxs` verifies copied exact NOEXCEPT pointers invoke a callable that handles a local exception, and rejects public named-parameter mismatch, reordered positional types, strengthening a throwing signature to NOEXCEPT, and value-to-reference return mismatch. One DUAL_TEST and four semantic rejection tests pass |
| Mixed procedure parameter categories | Reviewed | `main_test_183_mixed_procedure_parameters.qxs` covers interleaved named/positional signature identity, named-name distinction, split positional call groups, once-only written argument order, copied mixed procedure pointers and mutable reference result identity. Both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| Typed storage type sets | Reviewed | `main_test_182_storage_type_sets.qxs` checks multi-type TYPED_STORAGE identity under reordering and duplicate entries, explicit switching among scalar/owned/wide lifetimes, returning to the owned type, and rejection of an unlisted equal-width scalar. Two DUAL_TESTs and one semantic rejection test pass |
| Storage semantic constraints | Reviewed | `main_test_181_storage_constraints.qxs` adds eight semantic rejection tests for nonstorage expression/statement destinations, nominal initialization/destruction mismatch, constant initialization/destruction views, insufficient byte size and insufficient alignment. A positive DUAL_TEST verifies WRITE storage initialization, returned pointer mutation, original-slot projection and destruction. Physical layout restrictions retain INCLUDE_IF for layoutless targets |
| Move-assignment punctuation | Reviewed | `main_test_179_move_assignment_spelling.qxs` retains scalar and owned string :< assignments as KNOWN_BROKEN. The parser and expression metadata label this spelling move assignment, but both cases independently fail because lowering finds neither OPERATOR:< nor OPERATOR:<RHS. `main_test_180_custom_move_assignment.qxs` verifies explicit OPERATOR:< and OPERATOR:<RHS dispatch, once-only written operand order and mutable source/destination identity in both modes. The spelling works for user-defined operators; built-in/generated behavior remains missing |
| Reserved argument names and calling-convention types | Reviewed | `main_test_178_keyword_arguments_and_callconvs.qxs` checks GENERIC_THIS, GENERIC_OTHER and LHS public-to-local binding, reordered direct/indirect call evaluation, and STDCALL procedure identity through aliases and versus CCALL/NOEXCEPT signatures. Both DUAL_TESTs pass; no STDCALL ABI invocation or Windows execution is claimed |
| THROW operand lifetimes | Reviewed | `main_test_177_throw_operand_lifetimes.qxs` checks successful operand evaluation and a replacement exception thrown during evaluation, temporary destruction before enclosing-local cleanup, correct payload selection, and current-exception reset after the handler. Both paths pass in one DUAL_TEST under constexpr and native macOS ARM64 execution |
| DELETE evaluation and allocated arrays | Reviewed | `main_test_176_delete_evaluation.qxs` checks a throwing operand leaves its object live, subsequent deletion evaluates the operand once before destruction, and deletion of a fixed-array pointer destroys all three owned elements in reverse index order. Both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| NEW constructor argument lifetimes | Reviewed | `main_test_175_new_argument_lifetimes.qxs` checks owned arguments remain live through construction and expire before the next statement, later argument failure destroys only completed temporaries without entering the target constructor, subsequent allocation succeeds, and DELETE ends the separate allocated-object lifetime. Both DUAL_TESTs pass; allocator reclamation on the failure path is not measured by these assertions |
| Explicit storage call forms | Reviewed | `main_test_174_explicit_storage_calls.qxs` checks positional PLACE expression and statement arguments and reuse after ordinary destruction; the DUAL_TEST passes both modes. Named DESTROY arguments are retained as KNOWN_BROKEN because generated invocation supplies a DESTROY receiver that cannot bind the parameterized user destructor, while the generated destruction candidate rejects the named argument |
| Placement constructor arguments | Reviewed | `main_test_173_placement_arguments.qxs` checks named-argument PLACE expression and statement forms, once-only destination evaluation before constructor arguments, explicit DESTROY destination evaluation, and successful slot reuse after a throwing argument without target construction/destruction. Both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| Loop condition exception lifetimes | Reviewed | `main_test_172_loop_condition_exceptions.qxs` checks throwing iterator FILTER and entry TEST operands, suppression of bodies and steps, and CONTINUE-driven body cleanup before a throwing POSTTEST. All three DUAL_TESTs verify temporary destruction before enclosing-local destruction and typed handler entry in constexpr and native macOS ARM64 execution |
| Runtime phase branch lifetimes | Reviewed | `main_test_171_runtime_branch_lifetimes.qxs` checks reverse branch-local cleanup before shared continuation and exception handling, enclosing-local lifetime, and cleanup of phase statements without ELSE. All three DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| Static selection of owned temporaries | Reviewed | `main_test_170_static_choose_lifetimes.qxs` checks both STATIC_CHOOSE selections construct only one owned operand, retain it through a consuming call, destroy it at statement completion, and destroy it before enclosing locals when the consumer throws; both DUAL_TESTs pass constexpr and native macOS ARM64 execution |
| MATCH guard and subject lifetimes | Reviewed | `main_test_169_match_guard_lifetimes.qxs` checks failed/successful guard temporary cleanup before proceeding, retained subject-expression lifetime through the selected arm, and cleanup of both kinds before exception handling. Normal guard continuation fails the alive-count assertion in both constexpr and native execution and remains KNOWN_BROKEN; throwing-guard cleanup is tested separately |
| VISIT retained temporaries on exception | Reviewed | `main_test_168_visit_exception_lifetimes.qxs` checks named-block and EXTEND-continuation subject-expression temporaries remain alive during visitation, then are destroyed exactly once before a typed handler on exception; both DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| Static-branch runtime lifetimes | Reviewed | `main_test_167_static_branch_lifetimes.qxs` checks reverse branch-local cleanup before normal continuation, enclosing cleanup after continuation, both branch instantiations on exception, and semantic exclusion of invalid nested unselected code; both DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| Assertion condition lifetimes | Reviewed | `main_test_166_assertion_temporaries.qxs` checks cleanup before the next statement for successful owned conditions and compound short-circuit conditions, plus destruction before outer-scope cleanup when condition evaluation throws; both DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| RETURN_UNEQUAL operand lifetimes | Reviewed | `main_test_165_return_unequal_temporaries.qxs` records owned operand construction, custom comparator invocation, reverse destruction, equal continuation, unequal return and throwing-comparator unwinding. Equal-path cleanup fails both constexpr and native execution and is KNOWN_BROKEN; unequal-return and throwing-comparator cleanup pass both modes |
| Numeric sequence loop boundaries | Reviewed | `main_test_164_numeric_loop_bounds.qxs` distinguishes FROM/TO inclusive and FROM/UNTIL exclusive endpoints for equal/reversed ranges, stepped overshoot and exactly reached bounds, and preserves captured start/end/stride despite body mutations; all three DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| Assertion message literals | Reviewed | `main_test_163_assertion_messages.qxs` verifies once-only condition evaluation with normal, empty and escaped message literals. Standalone macOS failure entrypoints both exit 1 and print only the failed condition. The runtime ASSERT_FAIL accepts the optional tag but does not read it; custom message rendering is not implemented or claimed as passing coverage |
| Parenthesized type expressions | Reviewed | `main_test_160_grouped_types.qxs` checks nested grouping, pointer-to-array versus array-of-pointer identity, grouped symbols followed by qualified names/template application, live array storage, and grouped procedure pointer signatures/calls; all three DUAL_TESTs pass static evaluation and native macOS ARM64 execution |
| Explicit and deduced member templates | Reviewed | `main_test_159_member_template_binding.qxs` separates explicit named type-template arguments from DECAY-based deduction. Explicit member templates reject implicit @THIS even without paired shorthand and remain KNOWN_BROKEN; the deduced comparison passes static evaluation and native macOS execution. Declaration-type assertions in known-broken fixtures 143 and 157 now use DECLTYPE rather than reference-preserving TYPEOF |
| Paired template shorthand | Reviewed | `main_test_158_paired_templates.qxs` checks `#[index_type:value_type]` against explicit INDEX/VALUE binding with reversed formal declaration order, asymmetric and nested types, complete qualifiers, free/member calls and mutable-reference return identity. Three DUAL_TESTs pass both modes; the member-template call rejects implicit @THIS and is retained as KNOWN_BROKEN |
| Explicit overload-selector expressions | Unsupported | `main_test_157_overload_selectors.qxs` retains three positive KNOWN_BROKEN DUAL_TESTs for explicit indices, selected function addresses, and empty selection on an unambiguous function. Direct calls reject temploid references as not a functum, and address-taking rejects them as non-object bindings. These expression paths are distinct from working PROCEDURE_REF assembly selection in fixture 155. Index/type rejection tests were removed because the earlier unsupported path could falsely satisfy them |
| Structured external assembly operands | Unsupported | `main_test_161_external_assembly.qxs` retains C and raw Mach-O linker symbol references to abs as a positive KNOWN_BROKEN native test. Reaching these operands aborts compilation with uncaught std::bad_variant_access. In `asm_procedure_from_symbol.cpp`, the ast2_extern branch reads the component as ast2_procedure_ref before resolving the external symbol; no runtime assertions execute |
| VMIR text-only parser tokens | Reviewed | Recursive keyword cross-check found ACF, GLOBAL, IVK and THREAD only in `parsers/vmir2.hpp`. Its callers are VMIR parser tests, not the Quxlang source declaration/expression parser. These are internal text-format tokens and do not require .qxs syntax fixtures |
| Structured assembly references | Reviewed | `main_test_155_assembly_references.qxs` covers OBJECT_REF page/page-offset global relocations, pointer identity and bidirectional writes, plus PROCEDURE_REF with explicit CCALL and concrete overload selection, tail-branching to a positional Quxlang function. Both native macOS ARM64 tests pass. The initial named-parameter target assumed declaration-order ABI placement incorrectly; positional parameters make the tested register contract explicit |
| Inline assembly | Unsupported | `main_test_153_inline_assembly.qxs` retains register-bound scalar inputs/results, scratch CLOBBER metadata, empty clobbers and a pointer-write body as a positive KNOWN_BROKEN native ARM64 test. The first call fails with no candidates because `functum_list_user_overload_declarations.cpp` explicitly returns an empty overload list for inline_function; `asm_procedure_from_symbol.cpp` separately rejects backend support. No inline instructions execute |
| TARGET string expression | Unsupported | `try_parse_expression.hpp` accepts TARGET with a string literal, but its co_generate overload immediately throws rpnx::unimplemented; intended target-string vocabulary and result contract still need source/specification evidence before assertions can be written |

## Validation record

- 2026-09-07: short-circuit review completed. Fifteen new `DUAL_TEST`s passed
  through the static evaluator. The native macOS suite reported **354 unit tests
  passed**. The interpreter now preserves a selected block destination while
  destructor frames execute; no VMIR lowering change was required.
- Commands: `QXC_TARGETS=macos-arm64 QXC_OUTPUT_DIR="$PWD/tmp/short-circuit-final"
  local/qxc-compile-testbundle.sh`, followed by
  `tmp/short-circuit-final/output/macos-arm64/tests`.

- 2026-09-07: eleven additional `DUAL_TEST`s in `main_test_42_literal_values.qxs`
  and `main_test_43_scalar_operations.qxs` passed static evaluation and native
  execution. The complete native suite reported **365 unit tests passed**.
  Command: `QXC_TARGETS=macos-arm64 QXC_OUTPUT_DIR="$PWD/tmp/audit-scalar-final"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-scalar-final/output/macos-arm64/tests`.

- 2026-09-07: nine additional `DUAL_TEST`s in `main_test_44_control_flow.qxs`
  passed static evaluation and native execution. The complete native suite
  reported **374 unit tests passed**. Command:
  `QXC_TARGETS=macos-arm64 QXC_OUTPUT_DIR="$PWD/tmp/audit-control-flow"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-control-flow/output/macos-arm64/tests`. No compiler implementation
  changes were made for this batch.

- 2026-09-07: six new generation `DUAL_TEST`s and two scalar-addition
  regressions were added. The source bundle fails static evaluation at
  `scalar_operation_tests::signed_addition_crosses_zero` (`zero == 0`).
  Command: `QXC_TARGETS=macos-arm64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-static-generation-final"
  local/qxc-compile-testbundle.sh`.
- For independent native validation, a copy at
  `tmp/audit-static-native-bundle` changes only the two scalar regressions to
  `UNIT_TEST`; all six generation tests remain `DUAL_TEST` and pass static
  evaluation. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-static-native-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-static-native-final"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-static-native-final/output/macos-arm64/tests`.
  All **382 native unit tests passed**. The source regressions remain
  `DUAL_TEST`; the full source bundle is not currently passing static evaluation.

- 2026-09-07: six call-binding `DUAL_TEST`s in
  `main_test_46_call_binding.qxs` pass static and native execution. Validation
  used `tmp/audit-call-bundle`, a current source-bundle copy with only the two
  known scalar-addition regressions changed to `UNIT_TEST`. Command:
  `QXC_TARGETS=macos-arm64 QXC_INPUT_DIR="$PWD/tmp/audit-call-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-call" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-call/output/macos-arm64/tests`. All **388 native unit
  tests passed**. The original scalar regressions remain enabled for static
  evaluation in the source bundle. No implementation changes were made.

- 2026-09-07: six lambda `DUAL_TEST`s were added in
  `main_test_47_lambda_lifetimes.qxs`. `returned_owned_capture` is rejected
  during compilation with `Lambda capture source is not available: text`.
  Initial validation command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-lambda-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-lambda" local/qxc-compile-testbundle.sh`.
- The remaining five lambda tests pass static evaluation and native execution.
  For this run, the temporary bundle omits only `returned_owned_capture` and
  changes the two known scalar-addition regressions to `UNIT_TEST`. Command:
  `QXC_TARGETS=macos-arm64 QXC_INPUT_DIR="$PWD/tmp/audit-lambda-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-lambda-remaining"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-lambda-remaining/output/macos-arm64/tests`. All **393 included
  native unit tests passed**. The source bundle retains all regressions as
  `DUAL_TEST`; this is not a successful full-source-bundle validation.

- 2026-09-07: five enum/flagset `DUAL_TEST`s in
  `main_test_48_enum_flagset_values.qxs` pass static and native validation.
  The temporary bundle `tmp/audit-enum-bundle` omits only the known
  `returned_owned_capture` rejection and changes the two known scalar-addition
  regressions to `UNIT_TEST`. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-enum-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-enum" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-enum/output/macos-arm64/tests`. All **398 included
  native unit tests passed**. Source regressions remain unchanged; this does
  not establish a passing full source bundle.

- 2026-09-07: six mutation `DUAL_TEST`s in
  `main_test_49_mutation_values.qxs` pass static and native validation.
  Temporary bundle `tmp/audit-mutation-bundle` omits only the known
  `returned_owned_capture` rejection and changes the two known scalar-addition
  regressions to `UNIT_TEST`. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-mutation-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-mutation" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-mutation/output/macos-arm64/tests`. All **404 included
  native unit tests passed**. The full source bundle still retains its known
  failing regressions.

- 2026-09-07: four pointer `DUAL_TEST`s were added in
  `main_test_50_pointer_values.qxs`. The static run remained active without
  completing and was stopped; adding an iteration assertion did not resolve
  the stall. Changing only `element_scaled_arithmetic` to `UNIT_TEST` in the
  temporary bundle allowed the other three new tests to pass static evaluation.
  That isolation run used Release `qxc tmp/audit-pointer-bundle
  tmp/audit-pointer-isolation macos-arm64` and completed within a 60-second
  subprocess limit.
- Native validation used `tmp/audit-pointer-native-bundle`, with the four new
  pointer cases and two known arithmetic regressions as `UNIT_TEST`, and the
  known rejected lambda test omitted. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-pointer-native-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-pointer-native"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-pointer-native/output/macos-arm64/tests`. All **408 included native
  unit tests passed**. All source pointer regressions remain `DUAL_TEST`.

- 2026-09-07: all 18 positive alias cases in
  `main_test_33_aliases.qxs` pass static and native evaluation. Sixteen were
  upgraded from STATIC_TEST to DUAL_TEST; two are new correctness tests.
  The global-alias mutation test restores its initial value. Validation copy
  `tmp/audit-alias-bundle` changes only the two known scalar regressions and
  `element_scaled_arithmetic` to UNIT_TEST, and omits the known rejected
  `returned_owned_capture` case. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-alias-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-alias" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-alias/output/macos-arm64/tests`. All **426 included
  native unit tests passed**. Source regressions remain enabled.

- 2026-09-07: four conditional-declaration `DUAL_TEST`s in
  `main_test_24_include_if.qxs` pass static and native validation. The temporary
  bundle `tmp/audit-conditional-bundle` changes the two known scalar regressions
  and `element_scaled_arithmetic` to UNIT_TEST and omits the known rejected
  `returned_owned_capture`. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-conditional-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-conditional-final"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-conditional-final/output/macos-arm64/tests`. All **430 included
  native unit tests passed**. The source bundle retains the known regressions.

- 2026-09-07: 16 positive composite/reflection tests were upgraded from
  STATIC_TEST to DUAL_TEST (three composite and 13 public-field cases). Two
  new composite tests cover mixed copied/borrowed storage and empty split/join
  boundaries. All pass static and native validation. Temporary bundle
  `tmp/audit-reflection-bundle` changes the two known scalar regressions and
  `element_scaled_arithmetic` to UNIT_TEST and omits the known rejected
  `returned_owned_capture`. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-reflection-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-reflection" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-reflection/output/macos-arm64/tests`. All **448 included
  native unit tests passed**. Known source regressions remain enabled.

- 2026-09-07: four handler-lifetime `DUAL_TEST`s in
  `main_test_51_handler_lifetimes.qxs` pass static and native validation.
  Temporary bundle `tmp/audit-handler-bundle` changes the two known scalar
  regressions and `element_scaled_arithmetic` to UNIT_TEST and omits the known
  rejected `returned_owned_capture`. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-handler-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-handler" local/qxc-compile-testbundle.sh`;
  executable: `tmp/audit-handler/output/macos-arm64/tests`. All **452 included
  native unit tests passed**. Known source regressions remain enabled.

- 2026-09-07: four array-lifetime DUAL_TESTs were added. The final
  static run fails in `nested_array_partial_cleanup` at
  `source[row][column].complete`, before the copy or any exception occurs.
  The other three tests pass static evaluation with only this new case changed
  to UNIT_TEST. The temporary bundle also retains prior isolation: two scalar
  regressions and `element_scaled_arithmetic` are UNIT_TEST and the rejected
  `returned_owned_capture` is omitted. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-array-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-array-isolated"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-array-isolated/output/macos-arm64/tests`. All **456 included native
  unit tests passed**. All source array cases remain DUAL_TEST.

- 2026-09-07: four serialization DUAL_TESTs pass static evaluation in
  `tmp/audit-serialization-bundle`, with prior known failures isolated (two
  scalar cases, pointer arithmetic and nested-array case are UNIT_TEST; rejected
  lambda case omitted). Native execution fails
  `nonbyte_signed_and_unsigned_storage` at `bytes[4] == 255`. Command:
  `QXC_TARGETS=macos-arm64 QXC_INPUT_DIR="$PWD/tmp/audit-serialization-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-serialization"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-serialization/output/macos-arm64/tests`.
- A diagnostic-only copy makes the new signed-storage case UNIT_TEST and
  changes its high-byte expectation from 255 to 15. That run, under
  `tmp/audit-serialization-diagnostic`, passes all 460 included native tests.
  This confirms the observed byte and validates the remaining three new cases;
  it is not a passing run of the source expectations. The source test remains
  DUAL_TEST with the original sign-extension assertion.

- 2026-09-07: four numeric-conversion DUAL_TESTs in
  `main_test_54_conversion_boundaries.qxs` pass static and native validation.
  A fifth regression, `fractional_literal_subtraction`, reproduces a compiler
  abort on `0.0 - 1.0` with `std::invalid_argument: not an integer literal: 0.0`.
  Exact reproduction log: `tmp/audit-conversion-crash.log` (exit 134).
- Validation copy `tmp/audit-conversion-bundle` omits that crash case and the
  prior rejected lambda; changes the two scalar, pointer arithmetic, and nested
  array regressions to UNIT_TEST; and changes the signed-serialization case to
  STATIC_TEST to preserve its constexpr assertions while excluding its known
  native failure. Command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-conversion-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-conversion-typed"
  local/qxc-compile-testbundle.sh`; executable:
  `tmp/audit-conversion-typed/output/macos-arm64/tests`. All **463 included
  native unit tests passed**. Every source regression remains enabled.

- 2026-09-07: two inheritance tests added and three existing positive
  constructor/type-trait tests upgraded to DUAL_TEST. Static evaluation aborts
  `repeated_base_storage_identity` with `pointer target not found in array`
  (`tmp/audit-inheritance.log`, exit 134). With that case UNIT_TEST in the
  temporary bundle, the other four pass static evaluation and all **468 included
  native unit tests pass**. Executable:
  `tmp/audit-inheritance-native/output/macos-arm64/tests`.
- A diagnostic copy restores the repeated-base DUAL_TEST and removes only
  `ASSERT(left<- != right<-);`; static compilation then succeeds
  (`tmp/audit-inheritance-comparison.log`). Source assertions remain intact.
  These copies also isolate prior failures: scalar, pointer arithmetic and
  nested-array cases are UNIT_TEST; signed serialization is STATIC_TEST;
  rejected lambda and fractional-literal crash cases are omitted.

- 2026-09-07: three owning-generic tests were upgraded from UNIT_TEST to
  DUAL_TEST and two handle/reference tests added in `main_test_12_interfaces.qxs`.
  `generic_owning_copy_test` and `generic_owning_type_order_test` independently
  abort constexpr evaluation with `slot 19 is not alive, but should be`
  (`tmp/audit-interface.log` and `tmp/audit-interface-isolated.log`).
  `interface_copy_and_null_swap` fails `first??` after swapping on both paths.
- The lifecycle and reference-rebinding tests pass static and native evaluation.
  Native execution also passes both owning-generic comparison cases before
  stopping on the interface swap (`tmp/audit-interface-native-run.log`). The
  validation copy made the three new failing static cases UNIT_TEST, in addition
  to previously recorded exclusions. Source cases remain DUAL_TEST. No passing
  full native-suite claim is made for this batch.

- 2026-09-07: four variant-selection DUAL_TESTs passed static evaluation and
  native macOS execution. The isolated bundle reported **473 unit tests passed**
  (`tmp/audit-variant.log`, `tmp/audit-variant-run.log`), with the previously
  documented regressions excluded only in the temporary validation copy.

- 2026-09-07: added `KNOWN_BROKEN` and `KNOWN_BROKEN_IF(condition)` test
  modifiers. Eleven confirmed regressions now retain their assertions under
  `KNOWN_BROKEN`; no temporary exclusions are needed for the full bundle.
  `main_test_56_known_broken.qxs` covers unconditional, true-conditional and
  false-conditional behavior for STATIC_TEST, UNIT_TEST and DUAL_TEST.
  The full macOS ARM64 bundle reported **698 static tests passed, 15 known
  broken** and **468 unit tests passed, 15 known broken**. Each skipped total
  includes four tag-behavior fixtures. Evidence: `tmp/known-broken-full.log`
  and `tmp/known-broken-full-run.log`.
  Independent `.qxs` probes with `KNOWN_BROKEN_IF(FALSE)` and `ASSERT(FALSE)`
  failed during static evaluation and native execution respectively, confirming
  false conditions do not suppress failures (`tmp/known-broken-static-false.log`,
  `tmp/known-broken-unit-false-run.log`).
  A separate all-skipped `.qxs` probe also passed with debug VMIR generation:
  **0 static tests passed, 5 known broken** and **0 unit tests passed, 8 known
  broken** (`tmp/known-broken-all-skipped.log`,
  `tmp/known-broken-all-skipped-run.log`). Its temporary bundle marks the four
  std static tests and six runtime unit tests known broken as well. Active std
  static-test debug emission separately encounters an existing unavailable
  dependency-set error; this probe does not claim to validate that path.

- 2026-09-07: five struct-lifetime DUAL_TESTs passed both constexpr and native
  macOS execution. Coverage includes failure at each copied field, complete-copy
  storage independence, ownership transfer through generated moves, field cleanup
  after a constructor-body exception, and destructor-body ordering. The original
  bundle reported **703 static tests passed, 15 known broken** and **473 unit
  tests passed, 15 known broken**. No additional known-broken tags were needed.
  Evidence: `tmp/audit-struct-lifetimes.log` and
  `tmp/audit-struct-lifetimes-run.log`.

- 2026-09-07: three constructor-delegation DUAL_TESTs passed constexpr and
  native macOS evaluation. Reversed delegate lists still initialize fields in
  declaration order; failure at either field skips later argument evaluation
  and owner-body execution while cleaning only completed subobjects. A direct
  base initializes before ordinary fields and is destroyed after them.
  The original bundle reported **706 static tests passed, 15 known broken**
  and **476 unit tests passed, 15 known broken**. Evidence:
  `tmp/audit-constructor-delegates.log` and
  `tmp/audit-constructor-delegates-run.log`.

- 2026-09-07: two additional DUAL_TESTs in
  `main_test_58_constructor_delegates.qxs` cover throwing argument expressions
  at the first and second field, plus the successful path through all delegates.
  Assertions distinguish argument evaluation, constructor entry, owner-body
  entry, and destruction of completed fields. Both paths passed: **708 static
  tests passed, 15 known broken** and **478 unit tests passed, 15 known broken**.
  Evidence: `tmp/audit-delegate-arguments.log` and
  `tmp/audit-delegate-arguments-run.log`.

- 2026-09-07: three comparison-boundary DUAL_TESTs passed both execution
  modes. A shared matrix checks seven operators against independent sorted-value
  indices, including signed extrema and unsigned maxima across 5-, 8-, 12-,
  16-, 32- and 64-bit integer types, plus finite F32/F64 operands.
  The full bundle reported **711 static tests passed, 15 known broken** and
  **481 unit tests passed, 15 known broken**. Evidence:
  `tmp/audit-comparison-boundaries.log` and
  `tmp/audit-comparison-boundaries-run.log`.

- 2026-09-07: two float-special-value DUAL_TESTs passed constexpr and native
  macOS execution. A 36-pair matrix for each of F32 and F64 distinguishes
  Quxlang total ordering from IEEE comparisons across negative infinity,
  negative one, both signed zeros, positive one and positive infinity.
  Additional assertions cover reciprocal signed zeros, NaN reflexivity under
  Quxlang operators, unordered IEEE comparisons in both operand positions,
  and NaN results from opposite infinities and infinity multiplied by zero.
  Full-bundle totals: **713 static tests passed, 15 known broken** and
  **483 unit tests passed, 15 known broken**. Evidence:
  `tmp/audit-float-special.log` and `tmp/audit-float-special-run.log`.

- 2026-09-07: three native UNIT_TESTs cover atomic fetch ADD/SUB/AND/OR/XOR,
  their void-returning forms, integer and Boolean compare-exchange, and expected
  argument writeback. RMW checks cover I32/U32/I64/U64 under all five supported
  ordering modes; integer CAS covers nine valid success/failure mode pairs.
  These are value-semantics tests, not evidence of inter-thread ordering.
  The full bundle reported **713 static tests passed, 15 known broken** and
  **486 unit tests passed, 15 known broken** on macOS ARM64. Evidence:
  `tmp/audit-atomic-values.log` and `tmp/audit-atomic-values-run.log`.

- 2026-09-07: three macOS-only atomic thread UNIT_TESTs passed. One thousand
  release/acquire handshake rounds check visibility of two ordinary payload
  fields without concurrent reads and writes. Two-worker fetch-add checks the
  final count and aggregate returned-ticket sum; two-worker compare-exchange
  checks retry progress using expected-value writeback. These tests exercise
  the ordering contracts but cannot exhaust possible thread schedules.
  Full-bundle totals: **713 static tests passed, 15 known broken** and
  **489 unit tests passed, 15 known broken**. Evidence:
  `tmp/audit-atomic-threads.log` and `tmp/audit-atomic-threads-run.log`.

- 2026-09-07: two macOS UNIT_TESTs verify nontrivial PER_THREAD objects.
  Concurrent workers mutate independent owned strings while their lifetimes
  overlap, and the parent retains its own string. Eight successive workers
  each observe fresh initialization. Destruction counters verify one TLS
  destructor call per worker before join returns. Full-bundle totals:
  **713 static tests passed, 15 known broken** and **491 unit tests passed,
  15 known broken**. Evidence: `tmp/audit-thread-local-objects.log` and
  `tmp/audit-thread-local-objects-run.log`.

- 2026-09-07: three global-identity DUAL_TESTs were added. Type-keyed owned
  strings, alias identity and repeated returned references pass both paths.
  The value-keyed mutable-global case fails constexpr evaluation with a
  read-only storage error and remains marked KNOWN_BROKEN. A temporary copy
  changes only that test to UNIT_TEST; all **494 unit tests passed, 15 known
  broken**, establishing that its assertions pass natively. With the source
  tag retained, the full bundle reports **715 static tests passed, 16 known
  broken** and **493 unit tests passed, 16 known broken**. Evidence:
  `tmp/audit-global-identity.log`, `tmp/audit-global-identity-native-run.log`,
  `tmp/audit-global-identity-final.log`, and
  `tmp/audit-global-identity-final-run.log`.

- 2026-09-07: four indirect-call DUAL_TESTs passed constexpr and native
  execution. They verify runtime target selection, named binding independent
  of source evaluation order, reference-parameter mutation, reference-return
  identity, and exception payload/caller cleanup through a procedure pointer.
  Full-bundle totals: **719 static tests passed, 16 known broken** and
  **497 unit tests passed, 16 known broken**. Evidence:
  `tmp/audit-procedure-calls.log` and `tmp/audit-procedure-calls-run.log`.

- 2026-09-07: two more DUAL_TESTs in `main_test_65_procedure_calls.qxs`
  cover positional calls through indexed pointer arrays, argument evaluation
  order, target replacement, independent pointer copies and swapping targets.
  Both execution modes passed. Full-bundle totals: **721 static tests passed,
  16 known broken** and **499 unit tests passed, 16 known broken**. Evidence:
  `tmp/audit-positional-procedures.log` and
  `tmp/audit-positional-procedures-run.log`.

- 2026-09-07: two native macOS ARM64 assembly UNIT_TESTs passed. A ten-U64
  encoding procedure verifies declaration-order binding across the eight
  argument registers and two stack slots, including reversed named-call order.
  A second procedure mutates pointed-to storage and returns values above 32 bits.
  Full-bundle totals: **721 static tests passed, 16 known broken** and
  **501 unit tests passed, 16 known broken**. Evidence:
  `tmp/audit-arm64-calls.log` and `tmp/audit-arm64-calls-run.log`.

- 2026-09-07: two additional ARM64 UNIT_TESTs passed on macOS. Interleaved
  F32/I32 and F64/I64 calls verify independent integer/vector register assignment,
  negative integer operands, reordered named arguments and floating-point returns.
  A nine-double encoding procedure verifies the eighth-to-ninth argument boundary
  from vector registers to stack storage. Full-bundle totals: **721 static tests
  passed, 16 known broken** and **503 unit tests passed, 16 known broken**.
  Evidence: `tmp/audit-arm64-floats.log` and `tmp/audit-arm64-floats-run.log`.

- 2026-09-07: four runtime-selection tests use separate STATIC_TEST and
  UNIT_TEST assertions because their expected results intentionally differ by
  execution phase. Nested CONSTEXPR/NATIVE selection and static branches preserve
  the expected side-effect sequence; returns destroy only active branch locals
  before their enclosing local. All passed. Full-bundle totals: **723 static
  tests passed, 16 known broken** and **505 unit tests passed, 16 known broken**.
  Evidence: `tmp/audit-runtime-selection.log` and
  `tmp/audit-runtime-selection-run.log`.

- 2026-09-07: added template value-binding cases and upgraded four positive
  overload-ranking STATIC_TESTs to DUAL_TEST. Named, positional, mixed and
  nested-struct bindings pass both modes; all four ranking cases pass natively.
  The templated-namespace case triggers a compiler abort and remains enabled
  under KNOWN_BROKEN. Full-bundle totals: **725 static tests passed, 17 known
  broken** and **511 unit tests passed, 17 known broken**. Evidence:
  `tmp/audit-template-bindings.log`, `tmp/audit-template-bindings-final.log`,
  and `tmp/audit-template-bindings-final-run.log`.

- 2026-09-07: `main_test_69_checked_conversions.qxs` adds one DUAL_TEST
  for unsigned endpoints and six STATIC_TEST EXPECT_FAIL cases for out-of-range
  checked conversions, including negative-to-unsigned and non-byte widths.
  Full-bundle totals: **732 static tests passed, 17 known broken** and
  **512 unit tests passed, 17 known broken** (`tmp/audit-checked-conversions.log`,
  `tmp/audit-checked-conversions-run.log`).
  A temporary native version of the negative-to-unsigned case unexpectedly
  completes successfully. A second diagnostic makes the result observable and
  confirms it is U64 maximum. Its successful process exit demonstrates a missing
  fault, not correct checked-conversion behavior. Logs:
  `tmp/audit-checked-negative-native-run.log` and
  `tmp/audit-checked-negative-native-observed-run.log`.

- 2026-09-07: `main_test_70_rotation_counts.qxs` adds complete two-cycle
  rotation checks for U5/U8/U12/U16/U32/U64, with arithmetic expected values,
  inverse rotations and compound forms. These pass both modes. A large-count
  modulo-equivalence test passes constexpr but fails native execution at U5;
  it remains KNOWN_BROKEN. Final totals: **733 static tests passed, 18 known
  broken** and **513 unit tests passed, 18 known broken**. Evidence:
  `tmp/audit-rotation-counts.log`, `tmp/audit-rotation-counts-run.log`,
  `tmp/audit-rotation-counts-final.log`, and
  `tmp/audit-rotation-counts-final-run.log`.

- 2026-09-07: `main_test_71_logical_shifts.qxs` adds two DUAL_TESTs covering
  every valid right-shift count for six signed widths and both shift directions
  for five unsigned widths. Arithmetic expected values verify zero-fill,
  discarded bits, non-byte widths and compound forms. Standard-width large
  rotations were split into an active DUAL_TEST; only U5/U12 remain in the
  known-broken rotation case. All active cases passed both modes. Totals:
  **736 static tests passed, 18 known broken** and **516 unit tests passed,
  18 known broken**. Evidence: `tmp/audit-logical-shifts.log` and
  `tmp/audit-logical-shifts-run.log`.

- 2026-09-07: two enum-ordering DUAL_TESTs passed both modes. Explicit
  five-value enums declared out of numeric order exercise all comparison
  operators, including 64-bit unsigned high-bit values. A 256-pair matrix for
  a four-bit ALLOW_UNKNOWN enum includes named, unnamed and reserved values.
  Full-bundle totals: **738 static tests passed, 18 known broken** and
  **518 unit tests passed, 18 known broken**. Evidence:
  `tmp/audit-enum-ordering.log` and `tmp/audit-enum-ordering-run.log`.

- 2026-09-07: four union-selection DUAL_TESTs passed both modes. Boxed and
  inline unions with two named I32 alternatives exercise six alternating tag
  changes, mutable match projections, independent copies and reset to a VOID
  default. Guard tests cover skipped mismatched alternatives, first/second
  successful guards, OTHERWISE and DEFAULT fallback selection. Full-bundle
  totals: **742 static tests passed, 18 known broken** and **522 unit tests
  passed, 18 known broken**. Evidence: `tmp/audit-union-selection.log` and
  `tmp/audit-union-selection-run.log`.

- 2026-09-07: six union-lifetime DUAL_TESTs passed both modes. Boxed and
  inline unions destroy the old payload exactly once when changing named
  alternatives or resetting to VOID. Throwing from a matched arm destroys
  its local before the active payload. Failed payload copy construction
  preserves the source and avoids destruction of the incomplete copy.
  Full-bundle totals: **748 static tests passed, 18 known broken** and
  **528 unit tests passed, 18 known broken**. Evidence:
  `tmp/audit-union-lifetimes.log` and `tmp/audit-union-lifetimes-run.log`.

- 2026-09-07: three public-field reflection lifetime DUAL_TESTs passed both
  modes. A three-field owned temporary is evaluated once and remains alive
  throughout a call consuming its middle field. Both normal return and a
  throwing call destroy all fields in reverse declaration order. Moving the
  middle field through reflection transfers its ownership without moving the
  remaining fields or introducing copies. Full-bundle totals: **751 static
  tests passed, 18 known broken** and **531 unit tests passed, 18 known
  broken**. Evidence: `tmp/audit-reflection-lifetimes.log` and
  `tmp/audit-reflection-lifetimes-run.log`.

- 2026-09-07: three composite-call DUAL_TESTs passed both modes. APPLY
  preserves mutable reference identity through COMPOSITE_SPLIT,
  COMPOSITE_JOIN and COMPOSITE_FORWARD, including a reference-valued return.
  Forwarding an owned field into a value parameter performs no extra copy,
  clears source ownership, and destroys the parameter exactly once on normal
  return and exception propagation. Full-bundle totals: **754 static tests
  passed, 18 known broken** and **534 unit tests passed, 18 known broken**.
  Evidence: `tmp/audit-composite-calls.log` and
  `tmp/audit-composite-calls-run.log`.

- 2026-09-07: three return-lifetime DUAL_TESTs passed both modes. Early
  and trailing returns move either of two owned locals, clean up the other
  before returning, and leave exactly one owner with the caller. Explicit
  return-value copying is checked both on success and when its constructor
  throws, including reverse local destruction and no incomplete-result
  destructor. Existing reference-return identity assertions and
  RETURN_UNEQUAL operand/cleanup tests were rechecked; the listed return
  review gaps are closed. Full-bundle totals: **757 static tests passed,
  18 known broken** and **537 unit tests passed, 18 known broken**.
  Evidence: `tmp/audit-return-lifetimes.log` and
  `tmp/audit-return-lifetimes-run.log`.

- 2026-09-07: two conditional-lifetime DUAL_TESTs passed both modes.
  IF, ELSE UNLESS and nested IF/UNLESS destroy condition temporaries before
  entering the chosen arm or evaluating the next condition. Nested branch
  locals are destroyed in reverse nesting order. A throwing nested condition
  destroys its temporary before its enclosing branch local, skips both arms
  and preserves the exception payload. The listed conditional review gap is
  closed. Full-bundle totals: **759 static tests passed, 18 known broken**
  and **539 unit tests passed, 18 known broken**. Evidence:
  `tmp/audit-conditional-lifetimes.log` and
  `tmp/audit-conditional-lifetimes-run.log`.

- 2026-09-07: three labelled-exit DUAL_TESTs passed both modes. Breaking
  inner and outer labelled blocks checks the exact scope boundary and reverse
  interleaving of deferred actions with local destructors. GOTO from a catch
  handler cleans up the handler and enclosing scope, then restores the prior
  current-exception state. Existing forward/backward GOTO lifetime coverage
  also passed the full bundle; the listed label review gap is closed.
  Totals: **762 static tests passed, 18 known broken** and **542 unit tests
  passed, 18 known broken**. Evidence: `tmp/audit-label-lifetimes.log` and
  `tmp/audit-label-lifetimes-run.log`.

- 2026-09-07: three iterator-lifetime DUAL_TESTs passed both modes.
  Array ITEM/ITER projections identify the same storage; filtered mutation
  preserves skipped elements and CONTINUE advances each accepted iteration.
  FILTER temporaries are destroyed before body entry. Explicit STEP observes
  completed body cleanup after CONTINUE and is skipped after BREAK. Dynarr
  INDEX/VALUE projections preserve element identity during filtered mutation.
  Totals: **765 static tests passed, 18 known broken** and **545 unit tests
  passed, 18 known broken**. Evidence: `tmp/audit-iterator-lifetimes.log` and
  `tmp/audit-iterator-lifetimes-run.log`.

- 2026-09-07: two loop-clause lifetime DUAL_TESTs were added. POSTTEST
  temporary cleanup before STEP and after its final false result passes both
  modes. INIT/EVAL local destruction at loop exit fails both constexpr and
  native macOS at the zero-iteration assertion and is retained as KNOWN_BROKEN.
  Final full-bundle totals: **766 static tests passed, 19 known broken** and
  **546 unit tests passed, 19 known broken**. Evidence:
  `tmp/audit-loop-clauses.log`, `tmp/audit-loop-clauses-native-run.log`,
  `tmp/audit-loop-clauses-final.log`, and
  `tmp/audit-loop-clauses-final-run.log`.

- 2026-09-07: two nested-exception DUAL_TESTs passed both modes. A
  nested handler temporarily replaces CURRENT_EXCEPTION and restores the
  live outer payload on exit. A replacement throw unwinds handler-local
  deferred actions and objects; an EXCEPTION_PTR retains the original
  payload, can rethrow it inside the replacement handler, and destroys it
  only when the final handle leaves scope. Totals: **768 static tests passed,
  19 known broken** and **548 unit tests passed, 19 known broken**.
  Evidence: `tmp/audit-nested-exceptions.log` and
  `tmp/audit-nested-exceptions-run.log`.

- 2026-09-07: three deferred-capture DUAL_TESTs passed both modes.
  DEFER CALL owns a copied capture independently of later source mutation,
  invokes it before destruction on normal and exceptional exit, and accepts
  an explicitly moved closure without copying its captured payload again.
  A failed capture constructor does not register an action or destroy the
  incomplete payload; previously registered actions still execute.
  Totals: **771 static tests passed, 19 known broken** and **551 unit tests
  passed, 19 known broken**. Evidence: `tmp/audit-defer-captures.log` and
  `tmp/audit-defer-captures-run.log`.

- 2026-09-07: two handler-selection DUAL_TESTs passed both modes. A
  four-type dispatch matrix distinguishes I32, U32 and two nominal structures
  with identical fields. Nonmatching inner handlers propagate the original
  payload after reverse local/deferred cleanup. Existing mutable rethrow,
  default/sentinel selection, nested lexical rethrow and handled NOEXCEPT
  cases were rechecked; the listed exception-review gaps are closed.
  Totals: **773 static tests passed, 19 known broken** and **553 unit tests
  passed, 19 known broken**. Evidence: `tmp/audit-handler-selection.log` and
  `tmp/audit-handler-selection-run.log`.

- 2026-09-07: two owned-snapshot DUAL_TESTs expose compiler failures.
  Array-of-string snapshots fail constexpr pointer materialization; after
  that case was tagged, the single-string case aborts with a non-antestatal
  value diagnostic. Both retain expected version isolation and independent
  runtime-copy assertions under KNOWN_BROKEN. Final full-bundle totals:
  **773 static tests passed, 21 known broken** and **553 unit tests passed,
  21 known broken**. Evidence: `tmp/audit-owned-snapshots.log`,
  `tmp/audit-owned-snapshots-second.log`,
  `tmp/audit-owned-snapshots-final.log`, and
  `tmp/audit-owned-snapshots-final-run.log`.

- 2026-09-07: two conditional-object DUAL_TESTs passed both modes.
  Opposite Boolean template arguments select same-signature constructors and
  destructor bodies alongside owned or scalar field declarations. Tests
  verify active initialization, public reflection exclusion and owner-body
  destruction before the included owned field, with no excluded-field cleanup.
  Totals: **775 static tests passed, 21 known broken** and **555 unit tests
  passed, 21 known broken**. Evidence: `tmp/audit-conditional-objects.log`
  and `tmp/audit-conditional-objects-run.log`.

- 2026-09-07: two member-arithmetic DUAL_TESTs passed both modes.
  Distinct OPERATOR+ and OPERATOR+RHS implementations check written operand
  order and once-only evaluation, including the RHS fallback path. A custom
  OPERATOR+= mutates the original reference-valued receiver after evaluating
  receiver and scalar operand once. Totals: **777 static tests passed,
  21 known broken** and **557 unit tests passed, 21 known broken**.
  Evidence: `tmp/audit-member-arithmetic.log` and
  `tmp/audit-member-arithmetic-run.log`.

- 2026-09-07: two expression-grouping DUAL_TESTs passed both modes.
  Expected grouping follows the live expression parser: multiplication binds
  above addition/subtraction, which bind above division/modulus. Bitwise
  operations and shifts share a left-associative level above arithmetic.
  Noncommutative operands and explicit parentheses distinguish each tested
  grouping; subtraction and repeated division also check associativity.
  Totals: **779 static tests passed, 21 known broken** and **559 unit tests
  passed, 21 known broken**. Evidence: `tmp/audit-expression-grouping.log`
  and `tmp/audit-expression-grouping-run.log`.

- 2026-09-07: two floating-point subnormal DUAL_TESTs passed both modes.
  Exact repeated halving derives F32/F64 minimum normal and subnormal values,
  verifies every subnormal power-of-two step is positive and reversible, and
  checks arithmetic across the largest-subnormal/minimum-normal boundary.
  Final underflow rounds to the expected positive/negative zero, with IEEE
  comparisons and cancellation also checked. Totals: **781 static tests
  passed, 21 known broken** and **561 unit tests passed, 21 known broken**.
  Evidence: `tmp/audit-float-subnormals.log` and
  `tmp/audit-float-subnormals-run.log`.

- 2026-09-07: two member-comparison DUAL_TESTs passed both modes.
  A custom ordering ignores a deliberately reversed field and compares only
  its key across all 25 pairs of five values. All seven operators invoke the
  custom comparator exactly once per expression (175 total calls). Separate
  effectful operands verify left-to-right once-only evaluation for each
  operator. Totals: **783 static tests passed, 21 known broken** and
  **563 unit tests passed, 21 known broken**. Evidence:
  `tmp/audit-member-comparisons.log` and
  `tmp/audit-member-comparisons-run.log`.

- 2026-09-07: two member increment/decrement DUAL_TESTs passed both
  modes. Custom postfix members deliberately return transformed I32 values
  and change their receiver by different amounts, proving the syntax forwards
  the member result. Combined expressions preserve written mutation order;
  both value-used and discarded calls evaluate their receiver and invoke the
  member exactly once. Totals: **785 static tests passed, 21 known broken**
  and **565 unit tests passed, 21 known broken**. Evidence:
  `tmp/audit-member-incdec.log` and `tmp/audit-member-incdec-run.log`.

- 2026-09-07: a pointer increment/decrement DUAL_TEST passed both
  modes for U8, I32 and U64 elements. Each forward postfix result identifies
  the previous element and permits mutation of that original storage. Reverse
  traversal starts from one-past and checks every previous/result position,
  without dereferencing the endpoint or computing pointer differences.
  Totals: **786 static tests passed, 21 known broken** and **566 unit tests
  passed, 21 known broken**. Evidence: `tmp/audit-pointer-incdec.log` and
  `tmp/audit-pointer-incdec-run.log`.

- 2026-09-07: two custom-assignment DUAL_TESTs passed both modes.
  Observable OPERATOR:= and OPERATOR<-> bodies verify that syntax invokes
  custom behavior once after left-to-right operand evaluation. Self-assignment
  and self-swap retain the member's effects rather than being bypassed.
  Together with existing scalar, generated-record, move-ownership and pointer
  tests, these close the listed mutation review gaps. Totals: **788 static
  tests passed, 21 known broken** and **568 unit tests passed, 21 known
  broken**. Evidence: `tmp/audit-member-assignment.log` and
  `tmp/audit-member-assignment-run.log`.

- 2026-09-07: two default-argument lifetime DUAL_TESTs passed both
  modes. Owned defaults are evaluated in declaration order, remain alive
  during the call, and are destroyed in reverse order afterward. Failure at
  either factory skips the body and cleans only completed defaults. An
  explicit reference suppresses its throwing default and retains caller
  ownership. Totals: **790 static tests passed, 21 known broken** and
  **570 unit tests passed, 21 known broken**. Evidence:
  `tmp/audit-default-lifetimes.log` and
  `tmp/audit-default-lifetimes-run.log`.

- 2026-09-07: a heterogeneous owned-pack DUAL_TEST passed both modes,
  covering normal and exceptional exits. Record, integer and string entries
  are read using PACK_ARG, rebound to named mutable references, and moved
  into a second pack-taking call. The tracked record is copied once from its
  source and destroyed once after forwarding. The call parser supports named
  arguments and explicit positional groups but no general pack-spread form;
  no unsupported spread coverage is claimed. The listed supported-call review
  gaps are closed. Totals: **791 static tests passed, 21 known broken** and
  **571 unit tests passed, 21 known broken**. Evidence:
  `tmp/audit-owned-packs.log` and `tmp/audit-owned-packs-run.log`.

- 2026-09-07: two delegate-temporary DUAL_TESTs expose overlapping
  temporary lifetimes across field delegates. Both fail constexpr; the
  failure-path test also fails natively before the normal native test runs.
  Both retain their expected cleanup assertions as KNOWN_BROKEN. Final totals:
  **791 static tests passed, 23 known broken** and **571 unit tests passed,
  23 known broken**. Evidence: `tmp/audit-delegate-temporaries.log`,
  `tmp/audit-delegate-temporaries-normal.log`,
  `tmp/audit-delegate-temporaries-native-run.log`,
  `tmp/audit-delegate-temporaries-final.log`, and
  `tmp/audit-delegate-temporaries-final-run.log`.

- 2026-09-07: two closure-copy failure DUAL_TESTs passed both modes.
  Initial capture construction and copying an existing closure each throw
  at every position of three owned captures. Assertions check copy counts,
  exception payloads, reverse destruction of only the completed prefix, and
  survival of source objects. Failed copies leave the original closure
  callable; its captures are later destroyed once in reverse order.
  Totals: **793 static tests passed, 23 known broken** and **573 unit tests
  passed, 23 known broken**. Evidence: `tmp/audit-closure-copies.log` and
  `tmp/audit-closure-copies-run.log`.

- 2026-09-07: two lambda-signature DUAL_TESTs passed both modes.
  Explicit signatures combine renamed named parameters, a default, NOEXCEPT
  and an expression body while preserving written evaluation order. Reference
  returns identify borrowed source storage or independent owned capture
  storage, including copied-closure isolation. Parser review confirms explicit
  reference/value capture forms alongside implicit and empty capture lists;
  existing fixtures cover these. The escaping-owned-capture regression remains
  KNOWN_BROKEN. Totals: **795 static tests passed, 23 known broken** and
  **575 unit tests passed, 23 known broken**. Evidence:
  `tmp/audit-lambda-signatures.log` and `tmp/audit-lambda-signatures-run.log`.

- 2026-09-07: two nested-union DUAL_TESTs passed both modes across
  all four combinations of boxed/inline outer and inner storage. Moving an
  inner union leaves its source valueless, copying the outer union makes an
  independent owned payload, and replacing the original outer alternative
  destroys only its payload. Nested MATCH mutations survive independently
  until the copied owner is destroyed. Totals: **797 static tests passed,
  23 known broken** and **577 unit tests passed, 23 known broken**.
  Evidence: `tmp/audit-nested-unions.log` and `tmp/audit-nested-unions-run.log`.

- 2026-09-07: two union-assignment failure DUAL_TESTs passed both
  modes. The generated assignment takes OTHER by value, so its copy must
  finish before target mutation. A failed payload copy preserves both tags
  and payloads without destroying either; disabling the injected failure
  permits retry, replacement cleanup and independent later mutation.
  Both boxed and inline representations pass, closing the listed union review
  gaps. Totals: **799 static tests passed, 23 known broken** and **579 unit
  tests passed, 23 known broken**. Evidence: `tmp/audit-union-assignment.log`
  and `tmp/audit-union-assignment-run.log`.

- 2026-09-07: two typed-storage reuse DUAL_TESTs passed both modes.
  Three successive owned objects occupy the same TYPED_STORAGE slot, each
  receiving fresh construction state and exactly one explicit destruction.
  A throwing placement copy leaves the slot reusable; retry constructs a
  complete object without destroying the incomplete attempt. Exiting the
  storage's scope introduces no extra object destruction. Totals:
  **801 static tests passed, 23 known broken** and **581 unit tests passed,
  23 known broken**. Evidence: `tmp/audit-storage-reuse.log` and
  `tmp/audit-storage-reuse-run.log`.

- 2026-09-07: an allocated-storage DUAL_TEST passed both modes for
  successful construction and failure at each of three positions. The default
  allocator supplies typed slots; only completed objects are destroyed in
  reverse order, a cleared slot is reconstructed and destroyed, and a deferred
  action deallocates the storage afterward. Combined with local typed-storage
  reuse and array fixtures, this closes the listed storage lifetime review
  gaps; the existing nested-array constexpr regression remains KNOWN_BROKEN.
  Totals: **802 static tests passed, 23 known broken** and **582 unit tests
  passed, 23 known broken**. Evidence: `tmp/audit-allocated-lifetimes.log`
  and `tmp/audit-allocated-lifetimes-run.log`.

- 2026-09-07: a signed-bitwise DUAL_TEST passed both modes across
  I5/I8/I12/I16/I32/I64. A negative operand exercises the sign bits in all
  eight binary truth functions, every matching assignment form and unary
  complement, with explicit numeric expected values. Existing shift/count,
  rotation and grouping fixtures close the listed bitwise review gaps;
  the native non-byte large-rotation regression remains KNOWN_BROKEN.
  Totals: **803 static tests passed, 23 known broken** and **583 unit tests
  passed, 23 known broken**. Evidence: `tmp/audit-signed-bitwise.log` and
  `tmp/audit-signed-bitwise-run.log`.

- 2026-09-07: two active arithmetic-boundary DUAL_TESTs passed both
  modes for six signed and six unsigned widths. They exercise representable
  endpoint neighbors, minimum division/multiplication and maximum quotient/
  remainder reconstruction with compound operations. Signed minimum modulo
  two fails constexpr at I5 but passes natively across all six widths. That
  assertion was split into a separate KNOWN_BROKEN test, preserving active
  coverage for the remaining boundaries. Final totals: **805 static tests
  passed, 24 known broken** and **585 unit tests passed, 24 known broken**.
  Evidence: `tmp/audit-arithmetic-boundaries.log`,
  `tmp/audit-arithmetic-boundaries-native-run.log`,
  `tmp/audit-arithmetic-boundaries-final.log`, and
  `tmp/audit-arithmetic-boundaries-final-run.log`.

- 2026-09-07: exponentiation review added a passing custom-operator
  DUAL_TEST and a KNOWN_BROKEN built-in I32 expectation. Custom OPERATOR^
  dispatch and precedence above multiplication pass both modes. Built-in
  integer exponentiation fails semantic lookup before execution, with neither
  OPERATOR^ nor OPERATOR^RHS available; this is missing implementation, not a
  syntax failure. The listed arithmetic review gaps are closed with failures
  retained. Totals: **806 static tests passed, 25 known broken** and **586
  unit tests passed, 25 known broken**. Evidence: `tmp/audit-exponentiation.log`,
  `tmp/audit-exponentiation-final.log`, and
  `tmp/audit-exponentiation-final-run.log`.

- 2026-09-07: one new constrained-template DUAL_TEST and two existing
  template-kind tests upgraded to DUAL_TEST passed both modes. Exact type
  constraints select among integer widths and mutable/constant references,
  with canonical alias equivalence. Existing assertions verify whole-type
  binding, nested AUTO capture, TYPE/CLASS distinctions and reference argument
  deduction. Totals: **807 static tests passed, 25 known broken** and **589
  unit tests passed, 25 known broken**. Evidence:
  `tmp/audit-template-constraints.log` and
  `tmp/audit-template-constraints-run.log`.

- 2026-09-07: two method-binding DUAL_TESTs verify that copied bindings
  retain the original receiver through later calls and object swaps, and that
  signed and unsigned overloads remain selectable after binding. Binding copies
  use AUTO to preserve the binding type; DECLTYPE exposes its receiver type.
  Full macOS ARM64 bundle: **809 static tests passed, 25 known broken** and
  **591 unit tests passed, 25 known broken**. Evidence:
  `tmp/audit-method-bindings.log` and `tmp/audit-method-bindings-run.log`.

- 2026-09-07: replaced 30 temporary backend test exclusions with
  KNOWN_BROKEN_IF: 28 inheritance/dispatch/cast cases and two shared-ownership
  cases. The shared-pointer probe declaration is now available independently
  of the target. Extracted the LLVM-only half-precision assertion into its own
  conditional known-broken DUAL_TEST so unsupported targets count it as well.
  Physical size/alignment and raw allocator-layout tests retain INCLUDE_IF;
  OS-specific API and architecture-specific tests retain their target guards.
  Compiler evaluation and native macOS ARM64 execution passed: **810 static
  tests passed, 25 known broken** and **592 unit tests passed, 25 known broken**.
  Cortado execution was not run. Evidence: `tmp/audit-cortado-tags.log` and
  `tmp/audit-cortado-tags-run.log`.

- 2026-09-07: added three comparison DUAL_TESTs. Eight positive/negative
  signaling/quiet NaN encodings per F32/F64 canonicalize to one serialized
  quiet NaN, compare equal under total ordering, remain IEEE-unordered, and
  sort above infinity. All 64 encoding pairs per width exercise the seven
  comparison operators. Pointer coverage exercises all 25 pairs across four
  elements and the one-past endpoint for U8/I32/U64, without pointer subtraction.
  The listed comparison review is complete; the independent pointer-difference
  regression remains known broken. Full macOS ARM64 bundle: **813 static tests
  passed, 25 known broken** and **595 unit tests passed, 25 known broken**.
  Evidence: `tmp/audit-comparison-matrices.log` and
  `tmp/audit-comparison-matrices-run.log`.

- 2026-09-07: added four wide-flagset DUAL_TESTs. Active 63/64-bit cases
  exercise six representative masks and all 36 binary operand pairs, top-bit
  membership and integer conversion, reserved/unnamed bits, XOR assignment,
  guarded serialization and returned iterators. An implicit 63-bit declaration
  skips a reserved low range. Both active cases pass constexpr and native
  macOS ARM64. The 65/128-bit expectations remain KNOWN_BROKEN: an isolated
  65-bit probe reports the explicit 1..64 width restriction, while the 128-bit
  mask triggers an uncaught integer-overflow exception. The full bundle reports
  **815 static tests passed, 27 known broken** and **597 unit tests passed,
  27 known broken**. Evidence: `tmp/audit-wide-flagsets-final.log`,
  `tmp/audit-wide-flagsets-final-run.log`, `tmp/audit-wide-flagsets.log`,
  and `tmp/audit-wide-flagsets-probe.log`.

- 2026-09-07: added three generated-declaration DUAL_TESTs. Two active
  cases initialize distinct arrays in three STATIC_WHILE expansions, return or
  throw from each expansion, check completed/current local cleanup and skipped
  later blocks, and exercise repeated and non-exiting calls. A separate
  SNAPSHOT-dependent array-extent expectation is KNOWN_BROKEN: lookup loses
  the visible function-local static during type evaluation. An isolated
  UNIT_TEST copy also fails compilation with the same diagnostic, establishing
  that this is not limited to execution by the constexpr test interpreter.
  Full macOS ARM64 bundle: **817 static tests passed, 28 known broken** and
  **599 unit tests passed, 28 known broken**. Evidence:
  `tmp/audit-generated-declarations.log`, `tmp/audit-snapshot-extent.log`,
  `tmp/audit-generated-declarations-final.log`, and
  `tmp/audit-generated-declarations-final-run.log`.

- 2026-09-07: added two global-initialization DUAL_TESTs. A three-global
  dependency chain initializes in dependency order despite reverse declaration
  order, reuses its shared base without rerunning initialization, and preserves
  mutations through repeated returned references. A second global throws on
  its first initialization attempt, destroys its local, retries successfully
  on the next access and retains later mutation without another attempt.
  Both cases pass constexpr evaluation and native macOS ARM64 execution.
  Full-bundle totals: **819 static tests passed, 28 known broken** and
  **601 unit tests passed, 28 known broken**. Evidence:
  `tmp/audit-global-retry.log` and `tmp/audit-global-retry-run.log`.

- 2026-09-07: added three import/namespace DUAL_TESTs and a Quxlang fixture
  in the imported test library. Two renamed imports of the same module share
  type and mutable-global identity. Nested declarations and function aliases
  retain declaration-site lookup despite caller-local shadowing. Import aliases
  are module-wide: module_ast_impl merges file import maps, and the separate
  fixture 114 accesses imports declared in fixture 113 without its own preamble.
  Public methods can read their private members through renamed imports;
  directly calling an alias to a private function remains forbidden, as covered
  by the existing expected-failure fixture. Full macOS ARM64 validation:
  **822 static tests passed, 28 known broken** and **604 unit tests passed,
  28 known broken**. Evidence: `tmp/audit-import-lookup-final.log` and
  `tmp/audit-import-lookup-final-run.log`.

- 2026-09-07: added four option-selection DUAL_TESTs. The configured
  option_num value 5 propagates into dependent constants, a conditional type
  alias and array extents. Boolean options select one same-signature declaration,
  and string options initialize owned text. An expression-valued Boolean default
  fails with a kind-mismatch diagnostic and remains KNOWN_BROKEN: lowering only
  accepts literal default nodes. The normal macOS ARM64 bundle reports **825
  static tests passed, 29 known broken** and **607 unit tests passed, 29 known
  broken**. A temporary copy sets audit_option_enabled=true and audit_option_text
  to "configured text" in the existing target options map; an additional DUAL_TEST
  asserts those exact values and the enabled declaration result. That copy reports
  **826 static tests passed, 29 known broken** and **608 unit tests passed,
  29 known broken**. Repository configuration files were not changed. Evidence:
  `tmp/audit-option-selection.log`, `tmp/audit-option-selection-final.log`,
  `tmp/audit-option-selection-final-run.log`, `tmp/audit-option-override.log`,
  and `tmp/audit-option-override-run.log`.

- 2026-09-07: added four serialization DUAL_TESTs. U128 serialization
  verifies all sixteen bytes, and a nested scalar record verifies declaration
  order without layout padding, independent reconstructed fields, guard bytes
  and returned iterators. Both pass constexpr and native macOS ARM64. The
  isolated fixed-array case returns an unexpected output position in both
  modes; the combined record/array/U128 case fails constexpr with an array-bounds
  error. Both retain their assertions as KNOWN_BROKEN. The full bundle reports
  **827 static tests passed, 31 known broken** and **609 unit tests passed,
  31 known broken**. An isolated unit-only array probe fails its native
  `end == bytes[& 5]` assertion. Evidence: `tmp/audit-nested-serialization.log`,
  `tmp/audit-serialization-isolation.log`, `tmp/audit-serialization-final.log`,
  `tmp/audit-serialization-final-run.log`, and
  `tmp/audit-array-serialization-native-run.log`.

- 2026-09-07: added two STRINGLIKE DUAL_TESTs. A custom serializer emits
  130 payload bytes behind a multi-byte length prefix. Constant conversion
  preserves every byte, including leading/interior/trailing zeros and the UTF-8
  sequence C3 A9, and reaches END only after all 130 bytes. Owned string copies
  retain the explicit length and independent storage after tail replacement
  and append; constant storage also remains unchanged. Both cases pass constexpr
  and native macOS ARM64 execution. Full-bundle totals: **829 static tests
  passed, 31 known broken** and **611 unit tests passed, 31 known broken**.
  Evidence: `tmp/audit-stringlike-bytes.log` and
  `tmp/audit-stringlike-bytes-run.log`.

- 2026-09-07: added two atomic UNIT_TESTs. Atomic fields in adjacent
  array elements retain independent storage across reference-selected fetch-add,
  failed/successful compare-exchange and subtraction. The native macOS ARM64
  case passes. The pointer test retains null/live transitions and all nine
  success/failure mode pairs as KNOWN_BROKEN: no STORE overload is available
  for ATOMIC#(MUT->I32), so it cannot reach the comparisons. The storage-type
  predicate currently permits only integers, BYTE and BOOL. Full bundle:
  **829 static tests passed, 31 known broken** and **612 unit tests passed,
  32 known broken**. Evidence: `tmp/audit-atomic-pointers.log`,
  `tmp/audit-atomic-storage.log`, and `tmp/audit-atomic-storage-run.log`.

- 2026-09-07: added two aggregate procedure-pointer DUAL_TESTs. A
  three-F64 record preserves field permutation and caller storage in both modes.
  A mixed U8/I32/F64/three-U64-array record checks every transformed field,
  independent nested storage and a second call; it fails constexpr evaluation
  with an array-bounds error and remains KNOWN_BROKEN. A temporary unit-only
  version passes natively, reporting **614 unit tests passed, 32 known broken**.
  The original bundle reports **830 static tests passed, 32 known broken** and
  **613 unit tests passed, 33 known broken**. These are Quxlang procedure calls,
  not a validation of foreign C aggregate ABI compatibility. Evidence:
  `tmp/audit-aggregate-procedures.log`,
  `tmp/audit-aggregate-procedures-native-run.log`,
  `tmp/audit-aggregate-procedures-final.log`, and
  `tmp/audit-aggregate-procedures-final-run.log`.

- 2026-09-07: added two heterogeneous-composite DUAL_TESTs. An owned
  string, borrowed array, integer and Boolean retain field types and reference
  identity across TIE, SPLIT, JOIN, FORWARD and APPLY. Mutations through the
  reconstructed argument pack reach the intended storage on repeated calls.
  SELECT independently copies an owned string while preserving an external
  integer reference and excluding an unselected field. Both tests pass constexpr
  and native macOS ARM64 execution. Full-bundle totals: **832 static tests
  passed, 32 known broken** and **615 unit tests passed, 33 known broken**.
  Evidence: `tmp/audit-heterogeneous-composites.log` and
  `tmp/audit-heterogeneous-composites-run.log`.

- 2026-09-07: added two loop-handler DUAL_TESTs. WHILE exits through
  inner handlers destroy try, deferred, handler and body locals in the expected
  order and restore an enclosing exception before each new iteration and after
  BREAK. A LOOP case verifies that CONTINUE reaches STEP only after cleanup
  and exception-state restoration, while BREAK skips STEP. Both pass constexpr
  and native macOS ARM64 execution. Full-bundle totals: **834 static tests
  passed, 32 known broken** and **617 unit tests passed, 33 known broken**.
  The separate INIT/EVAL scope regression remains known broken. Evidence:
  `tmp/audit-loop-handler-exits.log` and `tmp/audit-loop-handler-exits-run.log`.

- 2026-09-07: added three generic-lifetime DUAL_TESTs. Moving an owning
  generic transfers its concrete object without another copy or duplicate
  destruction. Copying an owning generic creates an independent concrete owner,
  with mutation isolation and one destruction per owner. Moved generic references
  retain their receiver while another reference is rebound. The tracked concrete
  type supplies its required comparison operation so lifetime assertions do not
  depend on forbidden pointer-field comparisons. All three cases pass constexpr
  and native macOS ARM64. Full-bundle totals: **837 static tests passed,
  32 known broken** and **620 unit tests passed, 33 known broken**. Existing
  generic-comparison and interface-swap regressions remain separately tagged.
  Evidence: `tmp/audit-generic-lifetimes.log` and
  `tmp/audit-generic-lifetimes-run.log`.

- 2026-09-07: added three approximate float-conversion DUAL_TESTs.
  Five consecutive integers around 2^24 and 2^53 test exact representable values
  and both directions of round-to-nearest-even ties, mirrored for negative values.
  U64 and I64 maxima test unsigned high-bit handling and rounding carries to the
  next power of two in both F32 and F64. All pass constexpr and native macOS
  ARM64. Built-in constructor review found no float-to-integer or cross-width
  float conversion overloads; no rounding contract was invented for those absent
  features. Full-bundle totals: **840 static tests passed, 32 known broken**
  and **623 unit tests passed, 33 known broken**. Evidence:
  `tmp/audit-float-rounding.log` and `tmp/audit-float-rounding-run.log`.

- 2026-09-07: added two reference-qualifier DUAL_TESTs. WRITE parameters
  target distinct scalars, aliased scalar storage and reversed array elements,
  preserving source write order and visibility through an existing const view.
  Const record/field/pointer views observe later owner mutations, while the
  record's stored mutable pointer retains pointee access and reflects rebinding.
  Both pass constexpr and native macOS ARM64 execution. Full-bundle totals:
  **842 static tests passed, 32 known broken** and **625 unit tests passed,
  33 known broken**. Evidence: `tmp/audit-reference-qualifiers.log` and
  `tmp/audit-reference-qualifiers-run.log`.

- 2026-09-07: added three virtual-base DUAL_TESTs. Active cases verify
  shared virtual-root pointer identity, recovery of both branches and the complete
  object, state preservation after an unrelated cast fails, null results and
  const-view identity across later mutations. Both pass constexpr and native
  macOS ARM64. The active tests retain KNOWN_BROKEN_IF(ARCH_IS_LAYOUTLESS) for
  backend feature gaps. A separate virtual-diamond copying expectation is
  KNOWN_BROKEN: the complete-object constructor exposes only its explicit I32
  overload and cannot accept @OTHER. No copied-object assertions execute.
  Full-bundle totals: **844 static tests passed, 33 known broken** and
  **627 unit tests passed, 34 known broken**. Evidence:
  `tmp/audit-virtual-base-casts.log`, `tmp/audit-virtual-base-casts-final.log`,
  and `tmp/audit-virtual-base-casts-final-run.log`.

- 2026-09-07: reviewed structure modifier parsing and added two DUAL_TESTs.
  A structure with NO_IMPLICIT_CONSTRUCTORS, NO_IMPLICIT_ASSIGNMENT and
  NO_DEFAULT_SWAP still invokes its explicit constructor, copy, assignment and
  swap implementations. A ROOTED structure records its own field address,
  permits reference mutation and verifies that address again during exceptional
  destruction. Both pass constexpr and native macOS ARM64. Remaining modifier
  gaps are now named explicitly in the inventory. Full-bundle totals:
  **846 static tests passed, 33 known broken** and **629 unit tests passed,
  34 known broken**. Evidence: `tmp/audit-struct-modifiers.log` and
  `tmp/audit-struct-modifiers-run.log`.

- 2026-09-07: added two IBC_STRUCT DUAL_TESTs. Differently sized fields
  with non-lexical declaration order retain their values across independent
  copying, assignment, swap and indirect by-value argument/result calls. Reflection
  preserves declaration order. A macOS ARM64-specific test confirms natural
  alignment and padded size for the chosen layout. Both pass constexpr and
  native execution. Full-bundle totals: **848 static tests passed, 33 known
  broken** and **631 unit tests passed, 34 known broken**. This does not prove
  compatibility with an independently compiled foreign function. Evidence:
  `tmp/audit-ibc-values.log` and `tmp/audit-ibc-values-run.log`.

- 2026-09-07: added one DUAL_TEST and three semantic expected-compilation-
  failure tests for copy modifiers. NOT_COPYABLE rejects generated copy and
  assignment while permitting move construction, verified in constexpr/native
  execution. NO_IMPLICIT_COPY incorrectly allows ordinary copy initialization;
  that expected-rejection test remains KNOWN_BROKEN. No syntax-error fixtures
  were added. Copy generation checks the unrecognized spelling NO_BUILTIN_COPY
  rather than the parsed NO_IMPLICIT_COPY tag. Full macOS ARM64 bundle:
  **851 static tests passed, 34 known broken** and **632 unit tests passed,
  34 known broken**. Evidence: `tmp/audit-copy-modifiers.log`,
  `tmp/audit-copy-modifiers-final.log`, and
  `tmp/audit-copy-modifiers-final-run.log`.

- Final structures: `main_test_129_final_structs.qxs` adds two DUAL_TEST cases
  and three semantic expected-compilation-failure cases. Final records retain
  independent value operations and can be stored as fields; final polymorphic
  leaves can inherit and override behavior. Direct, aliased and virtual final
  bases are rejected. The polymorphic case uses
  `KNOWN_BROKEN_IF(ARCH_IS_LAYOUTLESS)` for the existing Cortado feature gap.
  macOS ARM64 compiler and native validation passed with **856 static tests
  passed, 34 known broken** and **634 unit tests passed, 34 known broken**.
  Evidence: `tmp/audit-final-structs.log` and `tmp/audit-final-structs-run.log`.

- Phase dependencies: `main_test_130_phase_dependencies.qxs` adds three
  DUAL_TEST cases and one semantic expected-compilation-failure case. Procedures
  with opposite-phase ON_LOWER errors are referenced only by their permitted
  phase, through both direct calls and stored procedure pointers. Returns in
  both phase alternatives disconnect a shared lowering error. An ordinary
  IF(FALSE) still retains its callee's invalid constexpr lowering dependency.
  Validation passed with **860 static tests passed, 34 known broken** and
  **637 unit tests passed, 34 known broken** on macOS ARM64. Evidence:
  `tmp/audit-phase-dependencies.log` and
  `tmp/audit-phase-dependencies-run.log`.

- Native termination: added standalone default-PANIC and ASSERT(FALSE) entry
  points to `main_test_panic.qxs` and exercised all five existing failure entry
  points in `main_test_20_fusion_native_failures.qxs`. A temporary bundle at
  `tmp/audit-native-failures-bundle` adds seven executable outputs without
  changing the repository manifest. Each executable exited with code **1** and
  its expected diagnostic: explicit/default panic messages, assertion text,
  MATCH DEFAULT FAIL, valueless MATCH, wrong-alternative UNWRAP and valueless
  UNWRAP. These are separate process checks, not additional unit-test counts.
  The same compilation and generated suite passed **860 static tests, 34 known
  broken** and **637 unit tests, 34 known broken**. Evidence:
  `tmp/audit-native-failures.log`, `tmp/audit-native-failures-run.log`, and
  `tmp/audit-native-failures-suite-run.log`. No native termination regression
  was found.

- Atomic publication: `main_test_131_atomic_publication.qxs` adds two macOS
  native tests, each repeating 1,000 handoffs. One publishes two non-atomic
  payload fields through a release store, a second thread's relaxed FETCH_ADD,
  and the reader's acquire load. The other reads both fields after a failed
  compare-exchange with acquire failure ordering and checks expected-value
  writeback. Release acknowledgement prevents reuse while the reader is active.
  The native suite completed within its 30-second timeout, with **639 unit
  tests passed, 34 known broken**; compilation reported **860 static tests
  passed, 34 known broken**. Evidence: `tmp/audit-atomic-publication.log` and
  `tmp/audit-atomic-publication-run.log`. These runs exercise ordering behavior
  on macOS ARM64; they do not prove correctness under every possible schedule.

- Sequential consistency: `main_test_132_atomic_total_order.qxs` adds a
  macOS native test with two persistent workers and 1,000 rounds. Each worker
  stores one atomic and then reads the other using ATOMIC_SEQCST. Assertions
  reject both reads returning zero and reject unexpected values. Release/acquire
  phase and completion counters synchronize reset and result storage outside
  each round without ordering the two tested store/load pairs against each other.
  The suite completed within its 30-second timeout: **860 static tests passed,
  34 known broken** and **640 unit tests passed, 34 known broken**. Evidence:
  `tmp/audit-atomic-total-order.log` and `tmp/audit-atomic-total-order-run.log`.
  No forbidden outcome was observed; schedule coverage remains empirical.

- macOS external calls: `main_test_133_macos_external_calls.qxs` adds two
  native tests using CCALL EXTERN_PROCEDURE declarations for libSystem modf,
  frexp, ldexp and memmove. Signatures were checked against the installed macOS
  SDK headers. Tests assert exact positive/negative floating decomposition,
  integer exponent output, mixed-register scaling, and destination identity plus
  every byte and both guards during forward/backward overlapping copies.
  Validation passed with **860 static tests, 34 known broken** and **642 unit
  tests, 34 known broken**. Evidence: `tmp/audit-macos-external.log` and
  `tmp/audit-macos-external-run.log`. The macOS gate selects real OS APIs rather
  than excluding a temporary Cortado implementation gap.

- External aggregate returns: `main_test_134_macos_aggregate_returns.qxs`
  declares macOS ARM64 IBC_STRUCT representations of SDK div_t and lldiv_t and
  calls libSystem div/lldiv through CCALL. Both tests fail at their first positive
  quotient assertion and remain UNIT_TEST KNOWN_BROKEN with all negative and
  full-width assertions preserved. The initial and isolated second failures are
  recorded in `tmp/audit-macos-aggregate-run.log` and
  `tmp/audit-macos-aggregate-second-run.log`. After tagging, validation passed
  **860 static tests, 34 known broken** and **642 unit tests, 36 known broken**;
  see `tmp/audit-macos-aggregate-final.log` and
  `tmp/audit-macos-aggregate-final-run.log`.

- Integer type predicates: three DUAL_TEST cases were added in
  `main_test_135_integer_type_queries.qxs`. Two pass for six signed/unsigned
  widths, canonical aliases, BYTE signedness and nonintegral classification.
  The nonintegral IS_SIGNED case fails at IS_SIGNED(BOOL), with a diagnostic
  incorrectly naming BITS, and is retained as KNOWN_BROKEN according to the
  predicate contract in `docs/spec/spec-DRAFT.md`. Final macOS validation:
  **862 static tests passed, 35 known broken** and **644 unit tests passed,
  37 known broken**. Evidence: `tmp/audit-type-queries.log`,
  `tmp/audit-type-queries-final.log`, and `tmp/audit-type-queries-final-run.log`.
  A parser-to-fixture cross-check also identified allocation-region and TARGET
  gaps above; the audit is not complete merely because its original rows have
  mostly been reviewed.

- Region resize: `main_test_136_region_resize.qxs` adds two DUAL_TEST cases.
  Resizing a two-element storage region to the same count preserves pointer
  identity and both live values. The pointer/count evaluation-order regression
  fails constexpr and native execution and is retained as KNOWN_BROKEN.
  Native isolation used `tmp/audit-region-resize-native-bundle` with only that
  test changed to UNIT_TEST. Evidence: `tmp/audit-region-resize.log` and
  `tmp/audit-region-resize-native-run.log`. Final validation passed **863 static
  tests, 36 known broken** and **645 unit tests, 38 known broken**; see
  `tmp/audit-region-resize-final.log` and `tmp/audit-region-resize-final-run.log`.

- Region roundtrips: `main_test_137_region_roundtrips.qxs` adds two DUAL_TEST
  cases and one macOS UNIT_TEST. Single-object storage is ended and reopened
  across three initialized/destroyed lifetimes. Array storage exercises both
  explicit-count and omitted-count END_MULTI_ALLOC_REGION, independent elements,
  and reinitialization after reopening. Native address escape/discovery preserves
  an interior pointer and a const view observes writes through the rediscovered
  mutable pointer without altering neighboring values. Address laundering is
  explicitly rejected by the constexpr interpreter and is tested natively only.
  Validation passed **865 static tests, 36 known broken** and **648 unit tests,
  38 known broken**. Evidence: `tmp/audit-region-roundtrips.log` and
  `tmp/audit-region-roundtrips-run.log`. No new regression was found.

- Dynamic regions: `main_test_138_dynamic_regions.qxs` adds a native lifecycle
  test covering BEGIN/RESIZE/END_DYNAMIC_ALLOC_REGION, address-before-size
  once-only evaluation, unchanged extent and a nested typed object lifetime.
  It passes. A native PARENT_ALLOC_ADDRESS test fails its required ADDRESS
  return-type assertion for a storage-pointer operand and is KNOWN_BROKEN.
  Evidence: `tmp/audit-dynamic-regions-run.log`.
- Scope correction: `docs/disorganized_ideas/provenance.md` explicitly restricts
  allocation-region expressions to native code. Fixtures 136 and 137 now use
  UNIT_TEST for region operations rather than DUAL_TEST. Their earlier constexpr
  observations remain historical evidence, not a claim of supported constexpr
  semantics. This removes three static passes and one static known-broken count.
  Enforcement of the phase restriction remains a separate review item.
  Final validation: **862 static tests passed, 35 known broken** and **649 unit
  tests passed, 39 known broken**. Evidence: `tmp/audit-dynamic-regions-final.log`
  and `tmp/audit-dynamic-regions-final-run.log`.

- Region relocation: `main_test_139_region_relocation.qxs` adds two native
  cases. Empty-region source/destination/extent evaluation runs once in source
  order, and destination storage remains usable. Relocating a live I64 into
  destination storage without a live destination object fails the value assertion
  and is KNOWN_BROKEN. The fixture follows the trivial-relocation preconditions
  in `docs/disorganized_ideas/provenance.md` and does not destroy the source
  object after intended relocation. Evidence: `tmp/audit-region-relocation-run.log`.
  Final validation: **862 static tests passed, 35 known broken** and **650 unit
  tests passed, 40 known broken**; see `tmp/audit-region-relocation-final.log`
  and `tmp/audit-region-relocation-final-run.log`.

- Region phase restrictions: `main_test_140_region_phase_restrictions.qxs`
  adds three STATIC_TEST EXPECT_FAIL cases. Address escape and discovery are
  rejected during constexpr execution as specified. Ending and reopening an
  allocated single-storage region instead completes successfully; its required
  rejection is KNOWN_BROKEN. The test releases its storage if execution wrongly
  proceeds, so its failure is the expected-failure contract itself, not a leak
  or unrelated invalid access. Initial evidence: `tmp/audit-region-phases.log`.
  Final validation: **864 static tests passed, 36 known broken** and **650 unit
  tests passed, 40 known broken**; see `tmp/audit-region-phases-final.log` and
  `tmp/audit-region-phases-final-run.log`.

- Remaining region phases: `main_test_141_remaining_region_phases.qxs` adds
  four native-only rejection expectations for multi-region begin/end/resize,
  dynamic-region lifecycle, parent lookup and empty relocation. A temporary
  bundle changed these four cases to ordinary STATIC_TEST so all bodies could
  be checked in one compiler run. All four completed successfully (**868 static
  tests passed, 36 known broken**) and released their allocations, proving
  missing rejection for each tested body. Source expectations are therefore
  STATIC_TEST EXPECT_FAIL KNOWN_BROKEN. Probe evidence:
  `tmp/audit-remaining-region-phases-probe.log` and the corresponding temporary
  bundle at `tmp/audit-remaining-region-phases-bundle`.
  Final source validation: **864 static tests passed, 40 known broken** and
  **650 unit tests passed, 40 known broken**; see
  `tmp/audit-remaining-region-phases-final.log` and
  `tmp/audit-remaining-region-phases-final-run.log`.

- Final virtual methods: the declaration-parser cross-check found that
  STRUCT FINAL coverage did not exercise the distinct VIRTUAL(FINAL) option.
  `main_test_142_final_virtual_methods.qxs` adds two DUAL_TEST cases and two
  semantic expected-compilation-failure cases. A final root method remains
  callable through a base receiver while a distinct derived overload is allowed;
  a finalized override survives another derived level and copying. Attempts to
  override either final slot are rejected. The cases retain conditional known-
  broken tags for the existing layoutless-backend inheritance gap.
  macOS validation passed **868 static tests, 40 known broken** and **652 unit
  tests, 40 known broken**. Evidence: `tmp/audit-final-virtual.log` and
  `tmp/audit-final-virtual-run.log`. No new final-method defect was found.

- ENABLE_IF templates: `main_test_143_enabled_templates.qxs` adds a passing
  captured-type DUAL_TEST, an expected-compilation-failure test for no enabled
  captured-type candidate, and a KNOWN_BROKEN DUAL_TEST for value-template
  conditions. Captured I32/F64 types select distinct bodies while a disabled
  invalid body is ignored. Identically parameterized template declarations with
  differing ENABLE_IF conditions instead fail with "Ambiguous template
  instanciation" at the first call, before return-type/value assertions execute.
  The negative test uses captured types so this template ambiguity cannot mask
  the no-enabled-overload expectation. Initial evidence:
  `tmp/audit-enabled-templates.log`. Final validation: **870 static tests passed,
  41 known broken** and **653 unit tests passed, 41 known broken**; see
  `tmp/audit-enabled-templates-final.log` and
  `tmp/audit-enabled-templates-final-run.log`.

- Literal type syntax: `main_test_144_literal_types.qxs` adds one passing
  DUAL_TEST for exact numeric/string literal type identity and overload selection,
  plus two independently reproduced KNOWN_BROKEN positive capture tests.
  NUMERIC_LITERAL_ANY fails compilation as unimplemented numeric_literal_any_temploidic;
  STRING_LITERAL_ANY fails as unimplemented string_literal_any_temploidic.
  Mismatched-kind rejection tests were removed because the same unimplemented
  path could satisfy them without demonstrating correct rejection behavior.
  Evidence: `tmp/audit-literal-types.log` and `tmp/audit-literal-types-string.log`.
  Final validation passed **871 static tests, 43 known broken** and **654 unit
  tests, 43 known broken**; see `tmp/audit-literal-types-final.log` and
  `tmp/audit-literal-types-final-run.log`. The type-parser cross-check identified
  additional pending forms above, so broad completion remains unproven.

- DECAY capture: `main_test_145_decay_capture.qxs` adds four passing
  DUAL_TEST cases for independent temporary/lvalue value parameters, bare DECAY
  owned return deduction, and explicit MUT&/CONST& DECAY capture preserving
  receiver identity. Review of `argument_initialize_by_template.cpp` shows that
  bare DECAY does not bind a reference-shaped candidate; explicit reference
  qualification is required. Initial fixture assumptions that bare DECAY would
  retain lvalue references were corrected rather than tagged as compiler bugs.
  Final validation: **875 static tests passed, 43 known broken** and **658 unit
  tests passed, 43 known broken**. Evidence: `tmp/audit-decay-capture-final.log`
  and `tmp/audit-decay-capture-final-run.log`.

- Constant type identities: `main_test_146_constant_type_identity.qxs` adds
  a passing DUAL_TEST covering CSTRING_CONSTANT and DATA_CONSTANT aliases,
  distinction from each other and STRING_CONSTANT/NUMERIC_CONSTANT, and overload
  selection through typed null pointers. This verifies type identity without
  dereferencing nonexistent constants. Current constructor enumeration provides
  literal construction only for STRING_CONSTANT and NUMERIC_CONSTANT; the two
  additional families share readonly-constant span internals, but dedicated
  nonempty construction semantics were not found in the documentation.
  Validation: **876 static tests passed, 43 known broken** and **659 unit tests
  passed, 43 known broken**. Evidence: `tmp/audit-constant-types.log` and
  `tmp/audit-constant-types-run.log`. Nonempty byte-access behavior for these two
  types remains unproven; the passing identity test does not substitute for it.

- Runtime metadata: `main_test_147_runtime_metadata.qxs` adds two UNIT_TEST
  cases. Every test name is nonempty; four named fixtures prove alignment of
  unconditional, conditional and enabled known-broken flags without depending
  on a fixed total test count. Test metadata pointers are present, the selected
  stepping is within STEPPING_COUNT, and startup dispatch tables are available.
  The tests inspect metadata without recursively invoking test procedures or
  startup entries. These are native-output builtins, so STATIC_TEST is not used.
  Validation passed **876 static tests, 43 known broken** and **661 unit tests,
  43 known broken**; evidence: `tmp/audit-runtime-metadata.log` and
  `tmp/audit-runtime-metadata-run.log`.

- Initguards: direct guard tests were placed in the runtime module because
  INITGUARD cannot be named by ordinary source modules. Two UNIT_TEST cases in
  `modules/runtime/sources/test_initguards.qxs` specify abort/retry/completion and
  independence of separate guards. Compilation aborts with uncaught
  std::bad_optional_access before either state test can execute. Isolating the
  second case reproduces the same failure, and a temporary runtime test containing
  only `VAR guard INITGUARD;` confirms local guard construction is sufficient.
  Both state cases are KNOWN_BROKEN; no guard-state failure is claimed.
  Evidence: `tmp/audit-initguards.log`, `tmp/audit-initguards-second.log`, and
  `tmp/audit-initguard-construction.log` (minimal bundle:
  `tmp/audit-initguard-construction-bundle`). Final validation passed **876 static
  tests, 43 known broken** and **661 unit tests, 45 known broken**; see
  `tmp/audit-initguards-final.log` and `tmp/audit-initguards-final-run.log`.

- Iterator LIMIT: the statement-parser cross-check found no direct LIMIT
  fixture despite prior LOOP coverage. `main_test_148_loop_limits.qxs` adds three
  DUAL_TEST cases. Explicit-BY boundary/overshoot and header-order/frozen-limit
  cases independently abort constexpr with non-live-slot transition errors and
  remain KNOWN_BROKEN. The filtered/default-step CONTINUE cleanup case passes
  both modes. A temporary bundle changed all three to UNIT_TEST; all passed
  natively (**664 unit tests, 45 known broken**). Evidence:
  `tmp/audit-loop-limits.log`, `tmp/audit-loop-limits-second.log`, and
  `tmp/audit-loop-limits-native-run.log`. Final source validation passed **877
  static tests, 45 known broken** and **662 unit tests, 47 known broken**;
  see `tmp/audit-loop-limits-third.log` and `tmp/audit-loop-limits-final-run.log`.
- INITGUARD_LOCK classification: global-accessor lowering creates this internal
  token when acquiring a guarded initializer; interpreter/backend consumers
  complete or abort it. Existing global retry/cleanup fixtures exercise that
  path. Parser acceptance alone does not establish a separate user lock API.

- Variable tags: `main_test_149_constexpr_global_tags.qxs` adds two passing
  DUAL_TEST cases for CONSTEXPR_READABLE and CONSTEXPR_READWRITE declarations.
  Reads preserve initialization and reference identity; writes through a captured
  reference update the tagged global and restore its baseline. Source searches
  find these tag spellings in the variable parser and parser tests, but no
  semantic consumers. The fixture therefore covers accepted declarations and
  ordinary global behavior, not unimplemented tag-specific access enforcement.
  No documented additional restriction was invented for a rejection test.
  Validation: **879 static tests passed, 45 known broken** and **664 unit tests
  passed, 47 known broken**; evidence: `tmp/audit-constexpr-global-tags.log`
  and `tmp/audit-constexpr-global-tags-run.log`.

- 2026-09-07: changed 26 portable threading, synchronization and atomic tests so Cortado gaps remain registered through KNOWN_BROKEN_IF. Native-only physical-layout, allocator-internal and foreign-ABI exclusions remain INCLUDE_IF. macOS ARM64 validation passed with **879 static tests passed, 45 known broken** and **664 unit tests passed, 47 known broken**. Logs: `tmp/audit-cortado-test-tags.log` and `tmp/audit-cortado-test-tags-run.log`.

- 2026-09-07: added three DUAL_TESTs in `main_test_150_ignored_arguments.qxs` for `%IGNORED` and `%...IGNORED`. All passed static evaluation and native macOS ARM64 execution: **882 static tests passed, 45 known broken** and **667 unit tests passed, 47 known broken**. The initial default-argument probe incorrectly referenced another parameter; it was corrected to use declaration-context storage, consistent with existing default lookup semantics. No compiler defect was established by that rejected probe. Logs: `tmp/audit-ignored-arguments-final.log` and `tmp/audit-ignored-arguments-final-run.log`.

- 2026-09-07: added four DUAL_TESTs and six semantic expected-compilation-failure tests in `main_test_151_inherited_options.qxs`. Reviewed DEFAULT_FROM chains, all three option kinds, source overrides, declaration-context lookup, independent derived overrides, and invalid source/cycle/kind/value rejection. The normal bundle and temporary `tmp/audit-inherited-options-override-bundle` (derived option and expected value set to 31, source/siblings remaining 17) both passed: **892 static tests passed, 45 known broken** and **671 unit tests passed, 47 known broken**. Logs: `tmp/audit-inherited-options-final.log`, `tmp/audit-inherited-options-final-run.log`, `tmp/audit-inherited-options-override.log`, and `tmp/audit-inherited-options-override-run.log`. No compiler or committed manifest changes were made.

- 2026-09-07: added three native macOS UNIT_TESTs in `main_test_152_optional_externals.qxs` for OPTIONAL external procedures, including a missing weak import. Compiler and native execution passed: **892 static tests passed, 45 known broken** and **674 unit tests passed, 47 known broken**. LLVM emits ExternalWeakLinkage for optional procedures, and the Mach-O linker encodes BIND_SYMBOL_FLAGS_WEAK_IMPORT; binary inspection retains the intentionally absent symbol name. Logs: `tmp/audit-optional-externals.log` and `tmp/audit-optional-externals-run.log`. DEFAULTED was classified as unreachable parser metadata rather than a missing executable fixture.

- 2026-09-07: added a positive inline ARM64 assembly test in `main_test_153_inline_assembly.qxs`; compilation fails at the first call with no candidates because inline declarations are excluded from overload discovery. Retained as KNOWN_BROKEN without compiler changes. Added a passing DUAL_TEST in `main_test_154_external_type_identity.qxs` for external declaration identity and aliases without managed-object execution. Final macOS ARM64 results: **893 static tests passed, 45 known broken** and **675 unit tests passed, 48 known broken**. Failure log: `tmp/audit-inline-assembly.log`; final logs: `tmp/audit-assembly-external-types.log` and `tmp/audit-assembly-external-types-run.log`.

- 2026-09-07: added two native macOS ARM64 tests in `main_test_155_assembly_references.qxs` for structured data/function assembly references. The initial procedure probe crashed after assuming named Quxlang parameters use source declaration order at the ABI boundary; `ordered_routine_parameters` instead orders named arguments separately. Correcting the test target to positional parameters made both tests pass; no compiler bug is claimed from that initial probe. Final results: **893 static tests passed, 45 known broken** and **677 unit tests passed, 48 known broken**. Logs: `tmp/audit-assembly-references-final.log` and `tmp/audit-assembly-references-final-run.log`.

- 2026-09-07: added three DUAL_TESTs and three semantic expected-compilation-failure tests in `main_test_156_privacy_scope_lists.qxs`. Comma-separated PRIVATE scopes grant access to either listed namespace and nested descendants; mixed CLASS/friend access preserves member identity and mutation, while unlisted scopes remain excluded independently for each declaration. All cases passed: **899 static tests passed, 45 known broken** and **680 unit tests passed, 48 known broken**. Logs: `tmp/audit-privacy-scope-lists.log` and `tmp/audit-privacy-scope-lists-run.log`. No compiler changes were made.

- 2026-09-07: added three positive DUAL_TESTs in `main_test_157_overload_selectors.qxs`. Independently confirmed failure of empty direct selection, explicit indexed direct calls, and selected-address construction in `tmp/audit-overload-selectors.log`, `tmp/audit-overload-selectors-explicit.log`, and `tmp/audit-overload-selectors-addresses.log`. All remain KNOWN_BROKEN; removed semantic negative probes that could falsely pass because the expression path is unsupported. Final macOS ARM64 validation: **899 static tests passed, 48 known broken** and **680 unit tests passed, 51 known broken**. Logs: `tmp/audit-overload-selectors-final.log` and `tmp/audit-overload-selectors-final-run.log`. No compiler changes were made.

- 2026-09-07: added four DUAL_TESTs in `main_test_158_paired_templates.qxs`. Asymmetric/nested paired type identity, complete qualifiers, named binding independent of formal order, and free-function reference-return identity pass. The member-template invocation fails with `unknown named argument @THIS` and remains KNOWN_BROKEN; no root cause beyond the observed candidate mismatch is claimed. Array-valued shorthand uses whitespace around the separator to avoid the `:[` token; syntax-error coverage remains excluded. Final macOS ARM64 results: **902 static tests passed, 49 known broken** and **683 unit tests passed, 52 known broken**. Failure log: `tmp/audit-paired-templates.log`; final logs: `tmp/audit-paired-templates-final.log` and `tmp/audit-paired-templates-final-run.log`.

- 2026-09-07: added two DUAL_TESTs in `main_test_159_member_template_binding.qxs`. Explicit named type-template member calls reproduce the unknown-@THIS candidate failure independently of paired shorthand and remain KNOWN_BROKEN; DECAY-deduced member calls pass and mutate the original receiver across I32/I64 instantiations. Initial type assertions incorrectly used reference-preserving TYPEOF on local variables; corrected to DECLTYPE here and in known-broken fixtures 143 and 157. No AUTO/DECAY compiler defect is inferred from those assertion failures. Final macOS ARM64 results: **903 static tests passed, 50 known broken** and **684 unit tests passed, 53 known broken**. Explicit failure log: `tmp/audit-member-template-binding-final.log`; final logs: `tmp/audit-member-template-binding-tagged.log` and `tmp/audit-member-template-binding-tagged-run.log`.

- 2026-09-07: added three DUAL_TESTs in `main_test_160_grouped_types.qxs`. Parenthesized type identity, postfix qualified/template lookup after grouping, pointer/array storage distinctions, mutation visibility and callable-pointer copying all pass. Pointer tokens are separated from following parentheses/brackets to avoid compound-token ambiguity; no syntax-error fixtures were retained. Final macOS ARM64 results: **906 static tests passed, 50 known broken** and **687 unit tests passed, 53 known broken**. Logs: `tmp/audit-grouped-types.log` and `tmp/audit-grouped-types-run.log`.

- 2026-09-07: recursive parser keyword cross-check identified EXTERNAL as the remaining structured assembly operand without a bundle fixture. Added `main_test_161_external_assembly.qxs`; reaching C/raw-linker operands aborts the compiler with uncaught std::bad_variant_access, consistent with the incorrect variant extraction in the external-operand lowering branch. Retained as KNOWN_BROKEN without compiler changes. Classified VMIR-only ACF/GLOBAL/IVK/THREAD tokens separately from Quxlang source grammar; keyword presence remains only an inventory aid, not proof of comprehensive coverage. Final macOS ARM64 results: **906 static tests passed, 50 known broken** and **687 unit tests passed, 54 known broken**. Failure log: `tmp/audit-external-assembly.log`; final logs: `tmp/audit-external-assembly-final.log` and `tmp/audit-external-assembly-final-run.log`.

- 2026-09-07: cross-checked `iter_parse_number` and string/character literal parsing against fixture 42. Added two DUAL_TESTs in `main_test_162_decimal_spellings.qxs` for leading-zero decimal integers (including 8/9 and values above 32 bits), arithmetic on zero-prefixed literals, and exact F32/F64 fractions with leading/trailing zeroes. Both pass. Corrected the stale literal-row reference to a pending conversion review; cross-width float constructors were already classified as unimplemented. Final macOS ARM64 results: **908 static tests passed, 50 known broken** and **689 unit tests passed, 54 known broken**. Logs: `tmp/audit-decimal-spellings.log` and `tmp/audit-decimal-spellings-run.log`.

- 2026-09-07: added one DUAL_TEST and two standalone failure entrypoints in `main_test_163_assertion_messages.qxs`. The normal/empty/escaped message success cases pass. A temporary bundle at `tmp/audit-assertion-messages-bundle` adds separate executable outputs; custom and empty assertion failures both exit 1 with exactly `Assert failed: FALSE\n`, confirming that current runtime diagnostics ignore the optional tag. No passing custom-message rendering claim is made, and deliberate-failure entrypoints are not registered as unit tests. Suite results: **909 static tests passed, 50 known broken** and **690 unit tests passed, 54 known broken**. Logs: `tmp/audit-assertion-messages.log`, `tmp/audit-assertion-messages-run.log`, `tmp/audit-assertion-messages-custom-run.log`, and `tmp/audit-assertion-messages-empty-run.log`. No compiler/runtime implementation or committed manifest changes were made.

- 2026-09-07: added three DUAL_TESTs in `main_test_164_numeric_loop_bounds.qxs` for numeric sequence loops. Equal/reversed ranges, inclusive/exclusive exactly reached endpoints, non-dividing stride overshoot, and copies of mutable start/end/stride variables all pass. This checks the numeric sequence lowering separately from the iterator LIMIT regressions in fixture 148. Final macOS ARM64 results: **912 static tests passed, 50 known broken** and **693 unit tests passed, 54 known broken**. Logs: `tmp/audit-numeric-loop-bounds.log` and `tmp/audit-numeric-loop-bounds-run.log`. No compiler changes were made.

- 2026-09-07: added three DUAL_TESTs in `main_test_165_return_unequal_temporaries.qxs`. Unequal return and throwing custom-comparator paths preserve construction/comparison order and reverse operand/local cleanup. Equal continuation fails the expected destruction-before-continuation event sequence in both constexpr and isolated native execution, and remains KNOWN_BROKEN. The lowering creates its continuation as a child of the comparison scope; shared lowering warrants investigation, with no compiler changes made. Failure logs: `tmp/audit-return-unequal-temporaries.log` and `tmp/audit-return-unequal-native-run.log`. Final macOS ARM64 results: **914 static tests passed, 51 known broken** and **695 unit tests passed, 55 known broken**. Final logs: `tmp/audit-return-unequal-final.log` and `tmp/audit-return-unequal-final-run.log`.

- 2026-09-07: added two DUAL_TESTs in `main_test_166_assertion_temporaries.qxs` after the RETURN_UNEQUAL cleanup finding. Assertions correctly destroy successful-condition temporaries before continuation, preserve right-then-left cleanup for compound short-circuit conditions, and unwind a throwing condition before enclosing locals. Both modes pass, bounding the prior finding rather than assuming all statement conditions share it. Final macOS ARM64 results: **916 static tests passed, 51 known broken** and **697 unit tests passed, 55 known broken**. Logs: `tmp/audit-assertion-temporaries.log` and `tmp/audit-assertion-temporaries-run.log`. No compiler changes were made.

- 2026-09-07: revisited the remaining mixed type-parser audit entry. Split verified DECAY, nominal constant identity, runtime metadata and guard machinery from the unresolved source-construction contract. Current source confirms DATA_CONSTANT has an internal producer in serialized static-global initialization, while literal constructor enumeration supports only STRING_CONSTANT/NUMERIC_CONSTANT. Asked for the intended public constant constructors and TARGET string semantics; no speculative value assertions or implementation changes were added. This was a source/audit review only; latest executed validation remains **916 static tests passed, 51 known broken** and **697 unit tests passed, 55 known broken** from `tmp/audit-assertion-temporaries.log` and `tmp/audit-assertion-temporaries-run.log`.

- 2026-09-07: added two DUAL_TESTs in `main_test_167_static_branch_lifetimes.qxs`. Both STATIC_IF/STATIC_ELSE instantiations destroy selected runtime locals before normal continuation and before enclosing cleanup on exception; invalid calls inside unselected nested static branches are not semantically compiled. All cases pass. Final macOS ARM64 results: **918 static tests passed, 51 known broken** and **699 unit tests passed, 55 known broken**. Logs: `tmp/audit-static-branch-lifetimes.log` and `tmp/audit-static-branch-lifetimes-run.log`. The separate constant-construction/TARGET contract questions remain open; no assumptions about them were introduced.

- 2026-09-07: reviewed all five VISIT parser forms against fixture 32 and added two DUAL_TESTs in `main_test_168_visit_exception_lifetimes.qxs`. Named-block and EXTEND-continuation forms retain subject-expression temporaries during visitation, evaluate the subject once, and destroy its retained temporary exactly once before typed exception handling, including a nested continuation scope. Final macOS ARM64 results: **920 static tests passed, 51 known broken** and **701 unit tests passed, 55 known broken**. Logs: `tmp/audit-visit-exception-lifetimes.log` and `tmp/audit-visit-exception-lifetimes-run.log`. No compiler changes were made.

- 2026-09-07: reviewed the remaining Cortado exclusions. Temporary inheritance,
  ownership and portable threading gaps already use KNOWN_BROKEN_IF; remaining
  target exclusions cover physical layout, allocator internals, foreign APIs
  and platform-specific assembly. Added MATCH guard lifetime tests in fixture
  169. Normal guard continuation fails `alive == 0` in both constexpr evaluation
  and isolated native execution and remains KNOWN_BROKEN. Throwing-guard cleanup
  passes in both modes. Final macOS ARM64 validation: **921 static tests passed,
  52 known broken** and **702 unit tests passed, 56 known broken**. Evidence:
  `tmp/audit-match-guard-lifetimes.log`,
  `tmp/audit-match-guard-native-run.log`, `tmp/audit-match-guard-final.log`
  and `tmp/audit-match-guard-final-run.log`. No compiler changes or Cortado runs.

- 2026-09-07: added fixture 170 for STATIC_CHOOSE owned-operand lifetimes.
  Both selection directions preserve the chosen temporary through its consuming
  call and destroy only that operand on normal and exceptional exits. The two
  DUAL_TESTs pass in both modes. macOS ARM64 validation: **923 static tests
  passed, 52 known broken** and **704 unit tests passed, 56 known broken**.
  Logs: `tmp/audit-static-choose-lifetimes.log` and
  `tmp/audit-static-choose-lifetimes-run.log`. No implementation changes.

- 2026-09-07: added fixture 171 for RUNTIME phase-local lifetimes. Normal
  continuation, exception propagation and independent statements without ELSE
  destroy only active branch locals in reverse order at the expected boundary.
  Enclosing locals survive normal continuation and unwind before the caller
  handles a phase-specific exception. All three DUAL_TESTs pass. Final macOS
  ARM64 validation: **926 static tests passed, 52 known broken** and **707 unit
  tests passed, 56 known broken**. Logs: `tmp/audit-runtime-branch-final.log`
  and `tmp/audit-runtime-branch-final-run.log`. No implementation changes.

- 2026-09-07: added fixture 172 for exceptions from FILTER, TEST and
  POSTTEST. Entry conditions suppress the body and STEP; a throwing POSTTEST
  follows CONTINUE-driven body cleanup and suppresses STEP. Owned operands
  unwind before enclosing locals, with no live temporary at handler entry.
  All three DUAL_TESTs pass. macOS ARM64 validation: **929 static tests passed,
  52 known broken** and **710 unit tests passed, 56 known broken**. Logs:
  `tmp/audit-loop-condition-exceptions.log` and
  `tmp/audit-loop-condition-exceptions-run.log`. No implementation changes.

- 2026-09-07: added fixture 173 for placement constructor arguments.
  Expression and statement forms evaluate the destination once before named
  arguments. A throwing argument leaves the typed slot reusable and does not
  invoke the target constructor or destructor. Explicit destruction evaluates
  its destination once before invoking the destructor. Both DUAL_TESTs pass.
  macOS ARM64 validation: **931 static tests passed, 52 known broken** and
  **712 unit tests passed, 56 known broken**. Logs:
  `tmp/audit-placement-arguments.log` and
  `tmp/audit-placement-arguments-run.log`. No implementation changes.

- 2026-09-07: added fixture 174 for positional placement and named explicit
  destruction. Positional expression/statement forms pass constexpr and native
  execution. Named DESTROY arguments fail compilation: user overloads expect
  MUT& THISTYPE but receive DESTROY storage, and the generated destruction
  overload rejects @digit. This positive expectation remains KNOWN_BROKEN;
  no destructor-body execution is claimed. Final macOS ARM64 validation:
  **932 static tests passed, 53 known broken** and **713 unit tests passed,
  57 known broken**. Logs: `tmp/audit-explicit-storage-calls.log`,
  `tmp/audit-explicit-storage-final.log` and
  `tmp/audit-explicit-storage-final-run.log`. No implementation changes.

- 2026-09-07: added fixture 175 for NEW argument lifetimes. Normal
  construction retains its owned argument until statement cleanup; a later
  argument exception destroys the completed argument without entering the
  constructor. A subsequent allocation succeeds and its explicit deletion
  records a separate object lifetime. Both DUAL_TESTs pass; these assertions
  do not measure allocator reclamation on the failed path. macOS ARM64
  validation: **934 static tests passed, 53 known broken** and **715 unit
  tests passed, 57 known broken**. Logs: `tmp/audit-new-argument-lifetimes.log`
  and `tmp/audit-new-argument-lifetimes-run.log`. No implementation changes.

- 2026-09-07: added fixture 176 for DELETE operand evaluation and owned
  fixed-array destruction. A throwing pointer producer preserves the object;
  a later successful DELETE evaluates its operand once before destruction.
  Deleting an allocated fixed array destroys all three live elements in reverse
  index order. Both DUAL_TESTs pass. macOS ARM64 validation: **936 static tests
  passed, 53 known broken** and **717 unit tests passed, 57 known broken**.
  Logs: `tmp/audit-delete-evaluation.log` and
  `tmp/audit-delete-evaluation-run.log`. No implementation changes.

- 2026-09-07: added fixture 177 for THROW operand evaluation. Both a
  returned exception value and an exception thrown while producing that value
  release operand temporaries before enclosing locals and deliver the correct
  payload to the handler. CURRENT_EXCEPTION is active inside and cleared after
  the handler. The DUAL_TEST passes both paths in both modes. macOS ARM64
  validation: **937 static tests passed, 53 known broken** and **718 unit tests
  passed, 57 known broken**. Logs: `tmp/audit-throw-operand-lifetimes.log` and
  `tmp/audit-throw-operand-lifetimes-run.log`. No implementation changes.

- 2026-09-07: cross-checked uppercase parser literals against all testbundle
  .qxs sources. Added fixture 178 for previously absent reserved argument names
  GENERIC_THIS, GENERIC_OTHER and LHS, plus STDCALL procedure type identity.
  Direct and indirect binding/evaluation assertions and convention/NOEXCEPT
  distinctions pass. Remaining unmatched strings include keyword prefixes,
  the CON debug-only string, and separately recorded DEFAULTED, INITGUARD_LOCK
  and TARGET boundaries; token occurrence alone is not completeness evidence.
  No STDCALL ABI call was executed. macOS ARM64 validation: **939 static tests
  passed, 53 known broken** and **720 unit tests passed, 57 known broken**.
  Logs: `tmp/audit-keyword-arguments.log` and
  `tmp/audit-keyword-arguments-run.log`. No implementation changes.

- 2026-09-07: cross-checked punctuation strings in the source parsers
  against .qxs fixtures. Added fixture 179 for previously uncovered :< move
  assignment; both I32 and std::string independently fail operator lookup and
  remain KNOWN_BROKEN. The unmatched +!, -!, +~ and -~ strings occur only in
  commented-out precedence entries; @... explicitly raises a syntax error;
  %% appears in a lexer-symbol static assertion. No syntax-error tests were
  added. Final macOS ARM64 validation: **939 static tests passed, 55 known
  broken** and **720 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-move-assignment-spelling.log`, `tmp/audit-move-assignment-scalar.log`,
  `tmp/audit-move-assignment-final.log` and
  `tmp/audit-move-assignment-final-run.log`. No implementation changes.

- 2026-09-07: added fixture 180 to distinguish :< operator dispatch from
  missing built-in/generated behavior. Explicit left-member OPERATOR:< and
  right-member OPERATOR:<RHS both pass, preserving written evaluation order and
  the original mutable operand identities. Fixture 179 remains KNOWN_BROKEN
  for I32 and std::string, which supply neither operator. Updated the audit
  classification to avoid claiming the spelling is universally unsupported.
  Both new DUAL_TESTs pass. macOS ARM64 validation: **941 static tests passed,
  55 known broken** and **722 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-custom-move-assignment.log` and
  `tmp/audit-custom-move-assignment-run.log`. No implementation changes.

- 2026-09-07: added fixture 181 for explicit storage constraints. Eight
  STATIC_TEST EXPECT_COMPILATION_FAILURE cases exercise invalid destinations,
  nominal type mismatch, constant views and independent size/alignment bounds.
  Existing uncalled invalid-placement functions alone were not counted as
  rejection evidence. A positive DUAL_TEST verifies WRITE-reference placement
  and subsequent mutation/projection/destruction. All new tests pass. Physical
  size/alignment tests retain layoutless exclusions because they test native
  storage layout. macOS ARM64 validation: **950 static tests passed, 55 known
  broken** and **723 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-storage-constraints.log` and `tmp/audit-storage-constraints-run.log`.
  No implementation changes or syntax-error fixtures.

- 2026-09-07: added fixture 182 for multi-type TYPED_STORAGE lists,
  previously absent from testbundle fixtures. Ordering and repeated entries
  preserve set identity. One slot supports successive I32, owned record, U64
  and owned-record lifetimes with explicit destruction and exact destructor
  counts. An unlisted U32 remains rejected despite I32 being allowed. Two
  DUAL_TESTs and one semantic rejection test pass. macOS ARM64 validation:
  **953 static tests passed, 55 known broken** and **725 unit tests passed,
  59 known broken**. Logs: `tmp/audit-storage-type-sets.log` and
  `tmp/audit-storage-type-sets-run.log`. No implementation changes.

- 2026-09-07: added fixture 183 for mixed named/positional PROCEDURE
  signatures, complementing prior homogeneous signatures. Interleaving the
  signature declarations preserves identity, while renaming a public named
  parameter changes it. Indirect calls bind split positional groups correctly,
  preserve written evaluation order and return the original mutable reference
  through a copied pointer. Both DUAL_TESTs pass. macOS ARM64 validation:
  **955 static tests passed, 55 known broken** and **727 unit tests passed,
  59 known broken**. Logs: `tmp/audit-mixed-procedure-parameters.log` and
  `tmp/audit-mixed-procedure-parameters-run.log`. No implementation changes.

- 2026-09-07: added fixture 184 for procedure pointer compatibility.
  Exact NOEXCEPT pointer initialization, copying and indirect calls pass with
  an internally handled exception and cleared caller exception state. Four
  semantic rejection tests prevent named-parameter renaming, positional type
  reordering, NOEXCEPT strengthening and return-category mismatch. All new
  tests pass. macOS ARM64 validation: **960 static tests passed, 55 known
  broken** and **728 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-procedure-compatibility.log` and
  `tmp/audit-procedure-compatibility-run.log`. No implementation changes.

- 2026-09-07: added fixture 185 for active lifetime enforcement in
  multi-type storage. Seven STATIC_TEST EXPECT_FAIL cases reject empty/dead
  projection, overlapping construction, empty/double destruction and inactive
  type projection/destruction. All types are nominally permitted by their
  storage declarations, separating these execution-state checks from fixture
  181's compile-time constraints. The interpreter's storage init/deinit/PUN
  handlers explicitly enforce these states. All expectations pass. macOS
  ARM64 validation: **967 static tests passed, 55 known broken** and **728 unit
  tests passed, 59 known broken**. Logs: `tmp/audit-storage-active-lifetimes.log`
  and `tmp/audit-storage-active-lifetimes-run.log`. No invalid native execution
  or implementation changes.

- 2026-09-07: added fixture 186 to extend scalar TYPEOF side-effect
  coverage to owned construction, throwing calls and explicit placement.
  Type inspection does not execute operand constructors/destructors or the
  throwing callee. Placement inspection preserves empty storage for a real
  placement and subsequent explicit destruction. Both DUAL_TESTs pass.
  macOS ARM64 validation: **969 static tests passed, 55 known broken** and
  **730 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-unevaluated-owned-expressions.log` and
  `tmp/audit-unevaluated-owned-expressions-run.log`. No implementation changes.

- 2026-09-07: a parser-keyword cross-check limited to the tests module
  found IBC_ENUM only in runtime/syscall sources. Added fixture 187 with focused
  defaults, identity, ordering, width forms and all byte representations under
  ALLOW_UNKNOWN. Corrected an initial test assumption: an enum without DEFAULT
  requires explicit initialization; a separate semantic rejection test retains
  that rule. Two DUAL_TESTs and one rejection test pass. No foreign-ABI claim
  is made. macOS ARM64 validation: **972 static tests passed, 55 known broken**
  and **732 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-ibc-enum-values.log`, `tmp/audit-ibc-enum-final.log` and
  `tmp/audit-ibc-enum-final-run.log`. No implementation changes.

- 2026-09-07: added fixture 188 for associated ENUM/FLAGSET bodies,
  extending the prior associated flag constant. Owning-type aliases, factory
  lookup and constant/mutable methods retain nominal type and receiver identity.
  Flag methods coexist with generated membership fields, and a constant view
  observes mutations made through the original receiver. Both DUAL_TESTs pass.
  macOS ARM64 validation: **974 static tests passed, 55 known broken** and
  **734 unit tests passed, 59 known broken**. Logs:
  `tmp/audit-nominal-associated.log` and `tmp/audit-nominal-associated-run.log`.
  No implementation changes.

- 2026-09-07: added fixture 189 for ENUM/FLAGSET templates. Dependent
  enum values fail lookup of maximum, and dependent flagset widths independently
  fail lookup of width; both positive expectations remain KNOWN_BROKEN. A
  fixed-value enum parameterized by type does instantiate, preserves distinct
  nominal identities and maintains independent values. Its qualified enumerator
  references use parenthesized template arguments so :: does not become part
  of the type argument. The comparison DUAL_TEST passes. macOS ARM64 validation:
  **975 static tests passed, 57 known broken** and **735 unit tests passed,
  61 known broken**. Logs: `tmp/audit-nominal-templates.log`,
  `tmp/audit-nominal-template-flags.log`, `tmp/audit-nominal-template-verified.log`
  and `tmp/audit-nominal-template-verified-run.log`. No implementation changes.

- 2026-09-07: expanded validation to all eight configured targets after the
  JVM exception-allocation SIZEOF failure. Per-test JVM compilation isolated
  unsupported exception paths, inheritance operations, atomic operations and
  dynamic-region operations. These tests now use
  `KNOWN_BROKEN_IF(ARCH_IS_LAYOUTLESS)`; their bodies remain available for
  backend implementation work. Empty nontrivial array copies also reach the
  exception path; their physical-size assertion is separately guarded.
  The 64-bit atomic and rotation cases are split from narrower widths and use
  `KNOWN_BROKEN_IF(ARCH_IS_X86)` for missing non-native atomic lowering and the
  missing `__udivdi3` runtime symbol, respectively. Narrower cases remain active.
  Cortado dependency collection now omits LLVM unwind-ABI requirements, which
  previously broke even an empty runner. Generated test metadata tables are
  initialized by separate methods to avoid exceeding the JVM method-size limit
  in the combined class initializer. These are compiler corrections; exception
  support itself remains unimplemented on Cortado.
  Validation completed successfully with Release qxc: `linux-x64`,
  `linux-x64-glibc`, `linux-arm64`, `linux-x86`, `linux-z-arch`, `windows-x64`,
  `macos-arm64` and `jvm-jvm` all emitted their configured artifacts. Native
  compilation is recorded in `tmp/audit-native-targets-verified.log`; JVM
  compilation is recorded in `tmp/audit-jvm-verified.log`. macOS reported
  **976 static tests passed, 57 known broken** and
  **737 unit tests passed, 61 known broken**, with runtime output in
  `tmp/audit-native-targets-verified-run.log`. Other targets were compiled only.

- 2026-09-07: fixtures 190 and 191 cover multi-argument `[]` and `[&]`
  expressions, which the expression parser accepts as `expression_multibind`.
  The lowering evaluates the receiver followed by each positional index before
  invoking the selected member operator. Three DUAL_TESTs check two-coordinate
  binding, mutable/constant dispatch, compound-assignment evaluation and
  original storage identity. Two further DUAL_TESTs check owned index lifetimes
  on normal and exceptional exits for both bracket spellings. All five pass
  constexpr evaluation and native macOS execution; the throwing test is
  conditionally known broken on layoutless targets because exception lowering
  is unimplemented there.
  Fixture 192 isolates an unrelated default-construction failure for a fixed
  array member followed by a pointer field. Constexpr reports
  "initializing element out of bounds of array" from the interpreter's
  `array_init_element` operation. No index operators are needed to reproduce
  it. The retained DUAL_TEST is KNOWN_BROKEN; its exact body passes native
  execution when changed to UNIT_TEST in a temporary bundle. This identifies
  a constexpr-only failure without attributing its cause to VMIR generation
  versus interpreter state handling. The five indexing tests use borrowed
  array storage so their coverage remains independent of this regression.
  Native isolation command: `QXC_TARGETS=macos-arm64
  QXC_INPUT_DIR="$PWD/tmp/audit-array-member-native-bundle"
  QXC_OUTPUT_DIR="$PWD/tmp/audit-array-member-native"
  local/qxc-compile-testbundle.sh`, followed by the generated macOS tests.
  It reported **981 static tests passed, 57 known broken** and
  **743 unit tests passed, 61 known broken** with the regression enabled only
  as a native test. Logs: `tmp/audit-array-member-native.log` and
  `tmp/audit-array-member-native-run.log`. No compiler changes were made.
  The final source bundle compiled successfully for all eight configured
  targets using `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64,macos-arm64,jvm-jvm
  QXC_OUTPUT_DIR="$PWD/tmp/audit-index-all-targets"
  local/qxc-compile-testbundle.sh`. macOS reported
  **981 static tests passed, 58 known broken** and
  **742 unit tests passed, 62 known broken**. Final logs:
  `tmp/audit-index-all-targets.log` and `tmp/audit-index-all-targets-run.log`.
  Other targets were compiled only; `git diff --check` passed.

- 2026-09-07: fixtures 193 and 194 extend postfix unary coverage beyond the
  existing built-in Boolean and integer assertions. Two DUAL_TESTs verify
  distinct user-defined `OPERATOR??`, `OPERATOR?!`, `OPERATOR!!` and
  `OPERATOR#!!` dispatch, once-only receiver evaluation, both Boolean inputs,
  preservation of receiver values, owned receiver lifetime through invocation,
  destruction before continuation, and chaining into built-in Boolean negation.
  A third DUAL_TEST verifies all four operators destroy their temporary receiver
  before enclosing locals and handler entry when they throw. It uses
  `KNOWN_BROKEN_IF(ARCH_IS_LAYOUTLESS)` for the existing exception backend gap.
  All three passed constexpr and native macOS execution. The suite reported
  **984 static tests passed, 58 known broken** and
  **745 unit tests passed, 62 known broken**. JVM emitted its complete test JAR.
  Validation: `QXC_TARGETS=macos-arm64,jvm-jvm
  QXC_OUTPUT_DIR="$PWD/tmp/audit-unary-postfix"
  local/qxc-compile-testbundle.sh`, followed by the generated macOS tests.
  Logs: `tmp/audit-unary-postfix.log` and `tmp/audit-unary-postfix-run.log`.
  The other six configured targets also compiled successfully with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-unary-other-targets"
  local/qxc-compile-testbundle.sh`; log: `tmp/audit-unary-other-targets.log`.
  All eight targets emitted their configured artifacts. Only macOS artifacts
  were executed. `git diff --check` passed.
  No compiler implementation changes were made.

- 2026-09-07: fixtures 195 and 196 extend bracket syntax coverage to
  parameter binding and const constraints. Two DUAL_TESTs verify trailing
  default coordinates, explicit overrides, empty and nonempty positional packs,
  ordered argument evaluation and stable address-result identity for both `[]`
  and `[&]`. Six STATIC_TEST EXPECT_COMPILATION_FAILURE cases reject missing or
  extra required coordinates for both operators, assignment through a constant
  index result, and calling a mutable address-index operator through a constant
  receiver. The positive operator paths are independently exercised by fixture
  190, so the rejection expectations do not depend on an unsupported operation.
  All eight tests pass. macOS reports **992 static tests passed, 58 known broken**
  and **747 unit tests passed, 62 known broken**; JVM emitted its complete JAR.
  Validation: `QXC_TARGETS=macos-arm64,jvm-jvm
  QXC_OUTPUT_DIR="$PWD/tmp/audit-index-binding"
  local/qxc-compile-testbundle.sh`, followed by the generated macOS executable.
  Logs: `tmp/audit-index-binding.log` and `tmp/audit-index-binding-run.log`.
  All six remaining targets compiled successfully with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-index-binding-other-targets"
  local/qxc-compile-testbundle.sh`; log:
  `tmp/audit-index-binding-other-targets.log`. All eight configured targets
  emitted their artifacts; only macOS was executed. `git diff --check` passed.
  No compiler implementation changes were made.

- 2026-09-07: fixture 197 expands the array-member default-initialization
  regression into eight independent cases: extents 0, 1, 2 and 6 with the array
  either first or between scalar fields. Each case verifies default values,
  independent sentinel fields, and writes to every valid array element.
  Isolated constexpr compilation passes both zero-element cases and rejects
  all six nonempty cases with "initializing element out of bounds of array".
  The failure therefore does not require a following pointer field, the original
  extent of six, or one particular field position. The six failing expectations
  remain KNOWN_BROKEN; the empty cases remain active DUAL_TESTs.
  All eight unchanged bodies pass native macOS execution when temporarily made
  UNIT_TESTs. The temporary bundle reported **992 static tests passed, 58 known
  broken** and **755 unit tests passed, 62 known broken**. Commands used
  `QXC_INPUT_DIR="$PWD/tmp/audit-array-extents-native-bundle"`,
  `QXC_OUTPUT_DIR="$PWD/tmp/audit-array-extents-native"` and
  `QXC_TARGETS=macos-arm64` with `local/qxc-compile-testbundle.sh`, followed by
  the emitted test executable. Logs: `tmp/extent-audit/results.json`,
  `tmp/audit-array-extents-native.log` and
  `tmp/audit-array-extents-native-run.log`. No compiler changes were made;
  the exact VMIR/interpreter state cause remains unlocalized.
  Final validation compiled all eight configured targets with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64,macos-arm64,jvm-jvm
  QXC_OUTPUT_DIR="$PWD/tmp/audit-array-extents-verified"
  local/qxc-compile-testbundle.sh`. The source bundle reported
  **994 static tests passed, 64 known broken** and
  **749 unit tests passed, 68 known broken** on macOS. Final logs:
  `tmp/audit-array-extents-verified.log` and
  `tmp/audit-array-extents-verified-run.log`. Other targets were compiled only;
  `git diff --check` passed.

- 2026-09-07: fixtures 198 and 199 add focused custom `OPERATOR->`
  coverage beyond iterator consumption. Four DUAL_TESTs verify mutable versus
  constant result qualification, once-only receiver evaluation during compound
  assignment, address-taking of the wrapper without invoking its operator,
  dereferencing a pointer to the wrapper before custom dispatch, and independent
  target identity after an owned temporary borrowing wrapper is destroyed.
  Normal and throwing paths assert receiver destruction before continuation or
  enclosing cleanup and handler entry. Two semantic rejection tests ensure
  constant results cannot be assigned or rebound to mutable references.
  All six tests pass constexpr validation and applicable native execution;
  only the throwing test is conditionally known broken on layoutless targets.
  macOS reports **1000 static tests passed, 64 known broken** and
  **753 unit tests passed, 68 known broken**. JVM emitted its complete test JAR.
  Commands used `QXC_TARGETS=macos-arm64,jvm-jvm` and
  `QXC_OUTPUT_DIR="$PWD/tmp/audit-dereference-boundaries"` with
  `local/qxc-compile-testbundle.sh`, followed by the macOS test executable.
  Logs: `tmp/audit-dereference-boundaries.log` and
  `tmp/audit-dereference-boundaries-run.log`. No compiler changes were made.
  The other six targets also compiled successfully using
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-dereference-other-targets"
  local/qxc-compile-testbundle.sh`; log: `tmp/audit-dereference-other-targets.log`.
  All eight targets emitted their configured artifacts. Only macOS was
  executed. `git diff --check` passed.

- 2026-09-07: fixture 200 adds three DUAL_TESTs for explicit floating-point
  type spellings. F32E8 and F64E11 are identical to F32 and F64 through aliases,
  reference/pointer qualification, arithmetic, copied values and procedure
  signatures. Different exponent widths remain distinct types. The latter
  assertions do not claim arithmetic support for every representable format.
  Direct and copied indirect calls use unambiguous function names. Taking an
  overloaded functum address is rejected before signature matching and does
  not test float spelling equivalence.
  All three tests pass constexpr and native macOS execution. macOS reports
  **1003 static tests passed, 64 known broken** and
  **756 unit tests passed, 68 known broken**; JVM emitted its complete JAR.
  Validation: `QXC_TARGETS=macos-arm64,jvm-jvm
  QXC_OUTPUT_DIR="$PWD/tmp/audit-explicit-floats"
  local/qxc-compile-testbundle.sh`, followed by the emitted macOS tests.
  Logs: `tmp/audit-explicit-floats.log` and `tmp/audit-explicit-floats-run.log`.
  No compiler changes or new known-broken tags were needed.
  All six remaining targets compiled with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-explicit-floats-other-targets"
  local/qxc-compile-testbundle.sh`; log: `tmp/audit-explicit-floats-other-targets.log`.
  All eight targets emitted their artifacts. Only macOS was executed.
  `git diff --check` passed; the broken-test summary retains the same skip-tag
  inventory and now records these latest validation totals.
- The refreshed keyword cross-check distinguishes source spellings from string
  fragments and internal parsers. CON is present in a debug-only prefix probe;
  I32U32 occurs in a commented parser assertion; COMPOSITE_, HAVE_ and DETECT_
  are prefixes rather than standalone constructs. Additional FUSION_*,
  TABLEBRANCH, UNREACHABLE and GET_UNDERYLING_STORAGE strings belong to the VMIR
  text parser. These do not establish missing Quxlang source fixtures. Existing
  audit entries retain DEFAULTED, INITGUARD_LOCK and TARGET limitations.
  Keyword presence remains an inventory aid, not a proof of complete semantics.

- 2026-09-07: fixture 201 adds focused NO_IMPLICIT_DEFAULT_CONSTRUCTOR
  coverage. Three DUAL_TESTs verify that generated copy, move and assignment
  remain available; declared default constructors and defaulted user arguments
  still permit zero-argument construction; and explicit member delegates can
  initialize a required-argument field whose generated copies remain independent.
  Three semantic rejection tests cover default local declarations, empty
  construction expressions and containing objects that cannot default their
  member. All six pass static validation and the positive bodies pass native
  macOS execution. Results: **1009 static tests passed, 64 known broken** and
  **759 unit tests passed, 68 known broken**. JVM emitted its complete JAR.
  Validation used `QXC_TARGETS=macos-arm64,jvm-jvm` and
  `QXC_OUTPUT_DIR="$PWD/tmp/audit-default-ctor-modifier"` with
  `local/qxc-compile-testbundle.sh`, followed by the macOS executable.
  Logs: `tmp/audit-default-ctor-modifier.log` and
  `tmp/audit-default-ctor-modifier-run.log`. No compiler changes or additional
  known-broken tags were required.
  All six remaining targets compiled successfully with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64
  QXC_OUTPUT_DIR="$PWD/tmp/audit-default-ctor-other-targets"
  local/qxc-compile-testbundle.sh`; log: `tmp/audit-default-ctor-other-targets.log`.
  All eight configured targets emitted their artifacts. Only macOS was executed.
  `git diff --check` passed, and the broken-test summary was refreshed with
  the latest totals without changing its regression inventory.

- 2026-09-07: fixture 202 covers NONSTATIC local default initialization, copy,
  move, assignment, mutable aliases and independent local containing records.
  Both positive DUAL_TESTs pass constexpr and native execution. Function-local
  STATIC declarations of directly tagged types unexpectedly compile and execute
  with both implicit and explicit initialization; the two semantic rejection
  expectations are KNOWN_BROKEN. No compiler root cause is asserted.
  STATIC eligibility of a containing record remains unspecified, with no
  positive, negative or known-broken expectation retained for that decision.
  All eight configured targets compile successfully with
  `QXC_TARGETS=linux-x64,linux-x64-glibc,linux-arm64,linux-x86,linux-z-arch,windows-x64,macos-arm64,jvm-jvm`
  and `QXC_OUTPUT_DIR="$PWD/tmp/audit-nonstatic-retained"` using
  `local/qxc-compile-testbundle.sh`. Only macOS was executed:
  **1011 static tests passed, 66 known broken** and
  **761 unit tests passed, 68 known broken**.
  Logs: `tmp/audit-nonstatic-retained.log` and
  `tmp/audit-nonstatic-retained-run.log`. The failed untagged probes are recorded
  in `tmp/audit-nonstatic-unspecified.log` and `tmp/audit-nonstatic-final.log`.
  The broken-test summary includes both direct-type rejection expectations.

- 2026-09-07: fixture 203 adds two DUAL_TESTs for SIZEOF/ALIGNOF type
  operands. Array layout checks cover extents one and three and a nested
  two-by-three shape for BYTE, I16, I32, U64, F32 and F64. They verify multiplied
  size and preserved element alignment without constructing array objects.
  Owned construction inside TYPEOF leaves constructor/destructor events and
  live counts unchanged when used by either layout query. Result types agree
  with their value-specific numeric literals, matching the lowering contract.
  Physical array/record layout is excluded from layoutless targets using
  INCLUDE_IF. Both tests pass constexpr and native macOS execution with no new
  known-broken tags. All eight targets compile successfully.
  Validation used `local/qxc-compile-testbundle.sh` with all eight configured
  QXC_TARGETS and `QXC_OUTPUT_DIR="$PWD/tmp/audit-layout-operands-verified"`.
  Only the macOS artifact was executed. Results:
  **1013 static tests passed, 66 known broken** and
  **763 unit tests passed, 68 known broken**.
  Logs: `tmp/audit-layout-operands-verified.log` and
  `tmp/audit-layout-operands-verified-run.log`. `git diff --check` passed.

- 2026-09-07: fixture 204 adds eight semantic rejection tests for layout
  operands: SIZEOF and ALIGNOF each reject local object names, mutable-reference
  names, function names and namespace names. These correspond to object/reference
  and non-class-symbol checks in the existing lowering. A positive DUAL_TEST
  verifies that explicit DECLTYPE supplies the local object's type and preserves
  its value. All nine tests pass static validation; the positive body also
  passes native macOS execution. Physical-layout cases exclude layoutless
  targets so unavailable ALIGNOF cannot mask the operand-specific checks.
  All eight configured targets compile successfully. Results on macOS:
  **1022 static tests passed, 66 known broken** and
  **764 unit tests passed, 68 known broken**. No additional skips were needed.
  Validation used `local/qxc-compile-testbundle.sh` with macOS/JVM outputs in
  `tmp/audit-layout-constraints` and remaining-target outputs in
  `tmp/audit-layout-constraints-other-targets`. Only macOS was executed.
  Logs: `tmp/audit-layout-constraints.log`,
  `tmp/audit-layout-constraints-run.log` and
  `tmp/audit-layout-constraints-other-targets.log`. `git diff --check` passed.

- 2026-09-07: fixture 205 adds a BITS matrix at widths 1, 7, 8, 9, 15,
  16, 17, 31, 32, 33, 63, 64, 65, 127 and 128 for both signs. It checks
  exact declared widths and signed/integral classification without depending
  on physical size or constructing wide objects. BYTE, the existing I12/U12
  aliases and value-specific numeric-literal result identity are also checked.
  A second DUAL_TEST verifies DECLTYPE operands and that TYPEOF of a mutating
  call does not execute the call. Six semantic rejection cases cover object
  and reference names, BOOL, F32, pointers and a record containing an integer.
  All eight tests pass static validation; both positive bodies pass native
  macOS execution. All eight configured targets compile successfully, with
  no new conditional exclusions or known-broken tags.
  Results: **1030 static tests passed, 66 known broken** and
  **766 unit tests passed, 68 known broken** on macOS.
  Validation used `local/qxc-compile-testbundle.sh` with macOS/JVM outputs in
  `tmp/audit-integer-bit-counts` and other targets in
  `tmp/audit-integer-bit-counts-other-targets`. Only macOS was executed.
  Logs: `tmp/audit-integer-bit-counts.log`,
  `tmp/audit-integer-bit-counts-run.log` and
  `tmp/audit-integer-bit-counts-other-targets.log`. `git diff --check` passed.

## Findings awaiting their feature review

- `interface_copy_and_null_swap` copies an implementation handle, clears the
  original, then swaps the null and implemented handles. The destination
  remains null on both constexpr and native paths (`ASSERT(first??)` fails).
- Enabling constexpr execution of the existing `generic_owning_copy_test` and
  `generic_owning_type_order_test` exposes slot-lifetime compiler aborts. Both
  continue to pass natively. The source tests now retain DUAL_TEST coverage;
  no implementation fixes were made.

- Comparing pointers to two distinct, same-typed base subobjects aborts
  constexpr evaluation with `pointer target not found in array`.
  `inheritance_tests::repeated_base_storage_identity` passes natively. Removing
  only the pointer inequality allows its remaining constexpr assertions to
  pass, isolating the failure from base copying and member access.

- `VAR value F32 := 0.0 - 1.0;` aborts the compiler with an uncaught
  `std::invalid_argument` reporting `not an integer literal: 0.0`.
  `conversion_boundary_tests::fractional_literal_subtraction` retains the
  minimal positive-expression regression. Typed floating-point subtraction
  passes in the surrounding conversion tests; no implementation fix was made.

- `I12` value `0 - 1` serializes as bytes `FF FF` in constexpr but `FF 0F`
  natively on macOS ARM64. Iterator positions, adjacent bytes, and decoding
  remain correct in the diagnostic run. The positive regression
  `serialization_boundary_tests::nonbyte_signed_and_unsigned_storage` retains
  the sign-extended byte expectation, consistent with the existing I5
  serialization test. No implementation changes were made.

- Nested `[2][2]element` initialization fails to preserve the constructor-set
  `complete` flag in constexpr evaluation. The assertion in
  `array_lifetime_tests::nested_array_partial_cleanup` fails before any copy or
  exception. The same test passes natively. An explicit move constructor leaves
  moved-from elements complete, ruling out that fixture ambiguity. The other
  three array tests, including failures at every one-dimensional copy position,
  pass static evaluation. The exact implementation cause remains unconfirmed.

- `pointer_value_tests::element_scaled_arithmetic` stalls during static
  validation while passing native execution. The other three pointer tests
  pass static evaluation when that one case is native-only. The precise
  operation and implementation cause remain unconfirmed. Removing only the
  negative pointer-difference assertion in a temporary copy still timed out
  after 60 seconds (`tmp/audit-pointer-nonnegative.log`); that assertion alone
  does not explain the stall.

- A lambda factory with a local `std::string text` cannot return a nested
  lambda with `[=text]`: capture analysis rejects it with
  `Lambda capture source is not available: text`. The positive correctness
  regression `lambda_lifetime_tests::returned_owned_capture` remains enabled.
  This is a compilation rejection, so native execution of that test is not
  available. No compiler fix was made.
- Closure-copy tests explicitly use `DECLTYPE(original)` for their copied
  variable type. `VAR copied AUTO := original` preserves a reference and aliases
  the original; it does not test an independent closure copy.

- Signed I32 addition across zero differs between constexpr and native execution.
  `VAR negative I32 := (0 AS I32) - 6; VAR zero I32 := negative + 6;`
  fails `ASSERT(zero == 0)` in constexpr, but passes natively. Sequential
  additions of 1, 2, and 3 also reproduced failed cancellation during reduction.
  `main_test_43_scalar_operations.qxs` retains ordinary and compound addition
  regressions. This reproduces without STATIC_WHILE or SNAPSHOT; the exact
  implementation cause remains unconfirmed. No implementation fix was made.

- A proposed fractional-value check using `small AS F64`, where `small` is F32,
  was rejected with `Cannot cast MUT& F32 AS F64`. The passing literal tests do
  not claim float-to-float conversion coverage. Determine supported conversion
  modes during the conversion review; no implementation was changed here.

## Review basis

The inventory follows the live `parse_file` query, declaration and statement
dispatchers, expression parser, and operator tables. Declaration-only features
are assessed through their observable effects in `.qxs` tests. Platform-specific
implementations and unsupported backend behavior must not be counted as covered
by a successful macOS run.

- `global_identity_tests::value_argument_storage` declares
  `TEMPLATE(@key VALUE U64) VAR U64 := key`. Mutation in constexpr evaluation
  fails with `Error executing <store_to_ref>: storing into read-only object`.
  The same test passes natively, including equal typed/untyped template-value
  identity and isolation of distinct keys. The test retains its expected
  assertions under KNOWN_BROKEN; no compiler change was made for this finding.

- `template_binding_tests::nested_value_bindings` aborts compilation when a
  TEMPLATE declaration wraps a NAMESPACE containing a nested function template:
  `instanciation_reference resolved to unsupported symboid kind` for the outer
  instantiation. The equivalent STRUCT-wrapped test passes constexpr and native
  execution. The namespace case is retained as KNOWN_BROKEN; no implementation
  change was made. Template parameter defaults were separately inspected and
  are not accepted by `try_parse_template_declaration.hpp`; no syntax-error
  fixture was added for them.

- Native checked integer conversion does not enforce its range contract.
  `-1 AS CHECKED U64` faults in constexpr but produces U64 maximum on macOS
  ARM64, even when its result is observed. The LLVM `iconv` emitter in
  `quxlang/sources/llvm-backend.cpp` ignores `convtype` and only truncates or
  sign/zero extends. The permanent regression inputs are retained as
  STATIC_TEST EXPECT_FAIL cases in `main_test_69_checked_conversions.qxs`.
  Native expected-fault execution currently requires an isolated diagnostic:
  the normal UNIT_TEST runner cannot treat process termination as a pass.
  No compiler change or misleading passing native assertion was added.

- `rotation_count_tests::large_rotation_counts` finds that native U5 rotation
  by 65537 differs from rotation by `65537 % 5`. Constexpr correctly reduces
  the original count modulo the logical bit width. LLVM rotation emission
  truncates the count to the operand's integer width before passing it to
  `fshl`/`fshr`, changing the remainder for non-power-of-two widths. The test
  includes both rotation directions and compound forms, but the native run
  stops at the first U5 left-rotation assertion. It is retained as KNOWN_BROKEN;
  no compiler change was made.

- `loop_clause_lifetime_tests::initialization_scope` fails `events == 21`
  after a zero-iteration loop in both constexpr and native macOS execution.
  INIT/EVAL locals are not destroyed at the end of the loop scope. The shared
  ordinary-loop lowering in `co_vmir_generator2.hpp` emits clause declarations
  into the incoming block, creates the after-block with that state, and only
  restores lookup names when finishing the loop. The expected reverse cleanup
  assertions for both zero iterations and BREAK are retained as KNOWN_BROKEN.
  The first failure prevents the BREAK case from running in the diagnostic.
  No compiler implementation was changed.

- Owned snapshots fail in `main_test_85_owned_snapshots.qxs`.
  `array_string_versions` reports `antestatal pointer materialization cannot
  anchor pointer target` while compiling the static test. That diagnostic
  originates in `ir2_constexpr_interpreter.cpp`. With the array case skipped,
  `string_versions` aborts compilation with `constexpr value is not an
  antestatal value`, thrown by the accessor in `data/constexpr_types.hpp`.
  The exact failing stage of the latter and whether both share one root
  cause remain unverified. Native execution of these cases was not reached.
  Both are KNOWN_BROKEN; no implementation change was made.

- `delegate_temporary_tests::normal_cleanup` and `failed_delegate_cleanup`
  expect argument temporaries to expire after each field delegate completes.
  Both fail constexpr at `object.alive-> == 1` when constructing the second
  field: the previous delegate temporary remains alive. Native execution
  fails the same assertion in `failed_delegate_cleanup`; the runner stops
  there, so the normal case was not independently executed natively.
  Both are retained as KNOWN_BROKEN with per-delegate cleanup expectations.
  Assertions later in those tests are not verified by the failing runs.
  No compiler implementation was changed.

- `arithmetic_boundary_tests::signed_minimum_remainder` retains the
  expectation that an even signed minimum modulo two is zero. The initial
  combined test fails constexpr at its first width, I5, while the same full
  test passes native macOS for I5/I8/I12/I16/I32/I64. The remainder assertion
  is isolated as KNOWN_BROKEN; other endpoint operations remain active and
  pass. Exact interpreter root cause is not yet established, and failure at
  the first width does not independently establish failures at later widths.

- `exponentiation_tests::integer_powers` expects built-in I32 powers but
  compilation finds neither I32::OPERATOR^ nor I32::OPERATOR^RHS. The parser
  accepts the exponentiation operator and the custom member test passes both
  modes; the built-in operation is not implemented. The expected built-in
  behavior is retained as KNOWN_BROKEN and was not executed natively.

### Flagset widths above 64 bits are unimplemented

`wide_flagset_tests::nonbyte_word_boundary` retains a 65-bit implicit mask
expectation; `full_double_word` retains an explicit 128-bit high mask.
`flagset_info_impl` evaluates every mask as `std::uint64_t`, accumulates occupied
bits in that type, and explicitly restricts BITS to 1..64. The 65-bit fixture
therefore fails semantic evaluation. A decimal mask of 2^127 reaches
`str_to_int<std::uint64_t>` and aborts with an uncaught `std::out_of_range`.
Both tests keep their intended assertions under KNOWN_BROKEN; no native execution
of those bodies was reached. Supported 63/64-bit behavior passes in both modes.

### SNAPSHOT cannot resolve a local static in an array extent

`generated_declaration_tests::snapshot_array_extent` declares a visible
STATIC_VAR and uses SNAPSHOT(count) in an array type inside STATIC_WHILE.
Compilation reports `SNAPSHOT requires a visible function-local static: count`.
The original combined constexpr test and an isolated unit-only copy both fail
before the body can execute. The diagnostic is emitted by expression lowering
in `co_vmir_generator2::co_generate(expression_snapshot)` when its visible
static-binding lookup fails. The exact context propagation defect remains
unresolved. The regression is retained as KNOWN_BROKEN; generated local cleanup
with fixed array extents passes both execution modes.

### Option defaults reject constant expressions

`option_selection_tests::expression_option_default` expects the Boolean
expression `option_num == 5` to supply a Boolean option default. Compilation
instead reports that the default does not match the declared option kind.
`create_default_option_value` in co_vmir_generator2.hpp accepts only numeric
literal nodes, string literal nodes, and Boolean keyword nodes; it does not
evaluate other constant expressions. The expectation remains KNOWN_BROKEN.
Configured numeric, Boolean and string option paths pass the new tests.

### Fixed-array serialization does not produce the expected element sequence

`nested_serialization_tests::array_element_bytes` expects two U16 elements to
occupy four bytes. Its returned-output-position assertion fails both constexpr
and native execution. Generated serialization routes non-scalar types through
`co_generate_builtin_serialize_struct`; built-in classes have an empty generated
field list, suggesting missing array-element handling. This source observation
is a likely cause, not a complete diagnosis. The combined record/array/U128
fixture separately raises a constexpr array-bounds error whose exact relationship
remains unresolved. Both expectations are KNOWN_BROKEN. Standalone U128 and
nested scalar-record serialization pass in both modes.

### Atomic pointer operations are unimplemented

`atomic_pointer_tests::comparison_modes` cannot compile its first STORE on
ATOMIC#(MUT->I32). `is_valid_atomic_storage_type` accepts only int_type,
byte_type and bool_type, and atomic built-in overload discovery rejects other
storage types. The intended pointer identity, null transition and expected-value
writeback assertions remain under UNIT_TEST KNOWN_BROKEN. They have not executed.
The independent nested integer-atomic storage test passes natively.

### Mixed record procedure calls fail constexpr with an array-bounds error

`aggregate_procedure_tests::mixed_record_value_roundtrip` reports
`During constexpr evaluation: initializing element out of bounds of array`.
The same body passes natively, including all argument-copy, result-field and
independent-storage assertions. The exact failing operation and any relationship
to the combined serialization regression remain unresolved. The DUAL_TEST keeps
its intended assertions under KNOWN_BROKEN; the floating-record counterpart
passes both modes.

### Virtual diamond complete-object copying has no matching constructor

`virtual_base_cast_tests::copied_virtual_root` cannot initialize a
virtual_constructor_diamond from another instance. The compiler reports that
.FULLOBJECT_CONSTRUCTOR has only the explicit positional I32 constructor and
no @OTHER overload. The intended independent virtual-root and copied-object
cast assertions remain KNOWN_BROKEN. The exact constructor synthesis defect
has not been resolved. Casts on normally constructed virtual diamonds pass
constexpr and native tests.

### NO_IMPLICIT_COPY does not suppress generated copying

`copy_modifier_tests::implicit_copy_rejected` unexpectedly compiles and executes
its ordinary copy initialization. The parser recognizes NO_IMPLICIT_COPY, but
`class_requires_gen_copy_ctor_impl` tests NO_BUILTIN_COPY instead; no semantic
check of NO_IMPLICIT_COPY was found. The intended semantic rejection is retained
as STATIC_TEST EXPECT_COMPILATION_FAILURE KNOWN_BROKEN. NOT_COPYABLE controls
in the same fixture pass.

`macos_aggregate_return_tests::packed_integer_return` and `two_register_return`
fail native quotient assertions for eight- and sixteen-byte external C returns.
The SDK declares quotient then remainder, as represented by the fixture's
IBC_STRUCT fields; the tests pass their preceding size assertions. In
`llvm-backend.cpp`, `abi_passes_by_value` excludes these record types,
`llvm_returnable_output_slot_target` therefore declines to select their RETURN
slot, and `build_callable_abi` leaves that slot as an argument and uses a void
LLVM return. `callable_abi_from_asm_callable` uses this same path for the external
signature. This is inconsistent with the system routines' register aggregate
returns. Both tests remain KNOWN_BROKEN; no compiler fix was made during the
correctness-test audit.

`integer_type_query_tests::nonsigned_types` retains the expectation that
IS_SIGNED returns false for types that are not signed integers. The draft type
query section specifies a Boolean predicate over types; current lowering in
`co_vmir_generator2.hpp` rejects non-integer classes after its BYTE special case.
The first tested input, BOOL, is rejected with an unrelated BITS diagnostic.
This mismatch is recorded without changing implementation or the draft contract.

`region_resize_tests::operand_evaluation_order` fails in both execution modes.
The `expression_resize_multi_alloc_region` overload in `co_vmir_generator2.hpp`
generates `input.pointer`, then `input.newcount`, then generates `input.pointer`
again to return its value. A side-effecting pointer expression therefore runs
twice. The test preserves a once-only pointer-before-count expectation with an
observable event sequence. This is a shared lowering defect, not an interpreter
or native-backend-only issue. No implementation change was made.

`dynamic_region_tests::parent_pointer_result_type` fails natively because
PARENT_ALLOC_ADDRESS preserves the input storage-pointer type. The provenance
notes specify an ADDRESS result for both storage-pointer and ADDRESS inputs.
The lowering overload simply returns `co_generate_expr(input.pointer_or_address)`
without producing an ADDRESS-typed result. The retained test checks the result
type before attempting any use; no compiler change was made.

`region_relocation_tests::live_integer_moves` fails natively: the destination
value differs from the source integer after RELOCATE_REGION_OBJECTS. The
`expression_relocate_region_objects` lowering only evaluates its three operands
and returns the source value; it emits no relocation operation. The intended
value-transfer assertion remains KNOWN_BROKEN. No compiler implementation was
changed during this audit.

`region_phase_tests::region_end_rejected_in_constexpr` unexpectedly completes
successfully. The provenance notes restrict region operations to native code,
but single-region begin/end lowering emits ordinary pointer casts that the
constexpr interpreter executes. In contrast, address laundering has an explicit
constexpr rejection. The missing single-region restriction is retained as a
KNOWN_BROKEN semantic expectation, with no implementation change.

The four `remaining_region_phase_tests` cases extend the missing native-only
restriction beyond single-region casts. Multi/dynamic region lifecycles, parent
lookup and empty relocation all complete during constexpr execution. The tests
retain the native-only contract from the provenance notes rather than accepting
these accidental constexpr paths as supported behavior. Region metadata and
provenance instrumentation remain implementation gaps; passing value/order
checks do not demonstrate their enforcement.

`enabled_template_tests::value_condition_and_return_type` cannot select among
identically parameterized function templates using mutually exclusive ENABLE_IF
conditions. Compilation reports "Ambiguous template instanciation" on its first
call; this diagnostic originates in `templex_select_template.cpp`. Captured-type
function conditions do select correctly. The value-template case remains
KNOWN_BROKEN, preserving body and return-type expectations. No template-selection
implementation change was made.

`literal_type_tests::numeric_capture_values` and `string_capture_values` cannot
compile calls to user functions whose arguments use NUMERIC_LITERAL_ANY and
STRING_LITERAL_ANY. Both forms are accepted by the type parser, and pseudotype
matching contains handlers, but these call paths report their respective
unimplemented temploidic types. Exact NUMERIC_LITERAL_TYPE/STRING_LITERAL_TYPE
function arguments do work. Positive capture expectations remain KNOWN_BROKEN;
no claim of complete literal-capture lowering is made.

Runtime-module local INITGUARD construction aborts compilation with uncaught
std::bad_optional_access. A declaration-only temporary probe reproduces it, so
this is earlier than the runtime acquisition/state operations. The two retained
runtime-module guard-state tests are KNOWN_BROKEN pending constructible guard
storage. The exact failing optional access has not been localized, and no
implementation change was made.

`loop_limit_tests::exclusive_limit_and_overshoot` and
`header_order_and_frozen_limit` abort constexpr evaluation with transition slots
14 and 27 reported not alive, respectively. Both use explicit BY with LIMIT;
the default-step filtered case passes. All three original bodies pass native
execution. The two failing expectations remain KNOWN_BROKEN. The exact split
between VMIR lifetime-state generation and interpreter transition handling has
not been localized, so no backend-specific root cause is asserted.
