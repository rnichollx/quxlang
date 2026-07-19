// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_SOURCES_QUERIES_QUERY_HELPERS_HEADER_GUARD
#define QUXLANG_SOURCES_QUERIES_QUERY_HELPERS_HEADER_GUARD

#include <quxlang/bytemath.hpp>
#include <quxlang/data/enum_flagset_info.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace quxlang::enum_info_detail
{
    /** Returns the bits required to encode a nonnegative integer. */
    auto required_unsigned_bits(bytemath::sle_int_unlimited value) -> std::uint64_t;
    /** Encodes an integer in an ENUM's canonical representation. */
    auto encode_integer(bytemath::sle_int_unlimited value, enum_integer_format const& format) -> std::vector< std::byte >;
} // namespace quxlang::enum_info_detail

namespace quxlang::detail
{
    /** Stores an ENUM value while its canonical representation is being assigned. */
    struct enum_info_pending_value
    {
        std::string name;
        bool is_null = false;
        bool is_default = false;
        bool is_explicit = false;
        std::optional< bytemath::sle_int_unlimited > value;
    };

    /** Stores an ENUM reserved range while its format is being selected. */
    struct enum_info_pending_range
    {
        bytemath::sle_int_unlimited from;
        bytemath::sle_int_unlimited to;
    };

    /** Stores a FLAGSET value while its mask is being assigned. */
    struct flagset_info_pending_value
    {
        std::string name;
        bool is_explicit = false;
        std::optional< std::uint64_t > mask;
    };

    /** Implementation helpers for the output-binary-information query. */
    struct output_binary_information_helpers;
    /** Implementation helpers for the instanciation-tempar-map query. */
    struct instanciation_tempar_map_helpers;
    /** Implementation helpers for the static-test execution query. */
    struct run_static_test_helpers;
    /** Implementation helpers for the boolean interpretation query. */
    struct interpret_bool_helpers;
    /** Implementation helpers for ASM procedure type resolution. */
    struct asm_procedure_from_symbol_helpers;
    /** Implementation helpers shared by argument source-form queries. */
    struct argument_source_form_helpers;
    /** Implementation helpers for argument adaptation ranking. */
    struct argument_adaptation_rank_helpers;
    /** Implementation helpers for class-conversion initialization. */
    struct argument_initialize_by_class_conversion_helpers;
    /** Implementation helpers for template initialization. */
    struct argument_initialize_by_template_helpers;
    /** Implementation helpers for reference objectization binding. */
    struct bindable_by_reference_objectization_helpers;
    /** Implementation helpers for ensig argument initialization. */
    struct ensig_argument_initialize_helpers;
    /** Implementation helpers for functum overload selection. */
    struct functum_select_function_helpers;
    /** Implementation helpers for constexpr evaluation. */
    struct constexpr_eval_helpers;
    /** Implementation helpers for antestatal constexpr evaluation. */
    struct constexpr_eval_antestatal_helpers;
    /** Implementation helpers for constexpr boolean extraction. */
    struct constexpr_bool_helpers;
    /** Implementation helpers for constexpr unsigned extraction. */
    struct constexpr_u64_helpers;
    /** Implementation helpers for LLVM query output assembly. */
    struct output_llvm_input_helpers;
    /** Implementation helpers for target module option selection. */
    struct module_options_map_helpers;
    /** Implementation helpers for symbol template-parameter collection. */
    struct symbol_tempars_helpers;
} // namespace quxlang::detail

#endif // QUXLANG_SOURCES_QUERIES_QUERY_HELPERS_HEADER_GUARD
