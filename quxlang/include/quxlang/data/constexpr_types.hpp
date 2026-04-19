// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD
#define QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD

#include <quxlang/data/antestatal.hpp>
#include <quxlang/data/constexpr.hpp>
#include <quxlang/data/basic_types.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <string>

namespace quxlang
{
    /// Reserved constexpr result ID used for the evaluated expression value.
    inline constexpr std::uint64_t constexpr_primary_result_id = 0;

    /// Value returned by the constexpr v3 evaluator.
    using constexpr_value = rpnx::variant< antestatal_value >;

    /// Returns the antestatal payload stored in a constexpr v3 value.
    inline auto constexpr_value_as_antestatal(constexpr_value const& value) -> antestatal_value const&
    {
        if (!typeis< antestatal_value >(value))
        {
            throw compiler_bug("constexpr value is not an antestatal value");
        }
        return as< antestatal_value >(value);
    }

    /// Returns the mutable antestatal payload stored in a constexpr v3 value.
    inline auto constexpr_value_as_antestatal(constexpr_value& value) -> antestatal_value&
    {
        if (!typeis< antestatal_value >(value))
        {
            throw compiler_bug("constexpr value is not an antestatal value");
        }
        return as< antestatal_value >(value);
    }

    /// Function-local static object made visible to a constexpr v3 evaluation.
    struct constexpr_static
    {
        /// Declared type of the static object made visible to constexpr evaluation.
        type_symbol type;
        /// Current antestatal value used to initialize the evaluation-local object.
        constexpr_value value;
        /// Result ID used to return mutations, or nullopt when the static is read-only.
        std::optional< std::uint64_t > mutation_result_id;

        RPNX_MEMBER_METADATA(constexpr_static, type, value, mutation_result_id);
    };

    /// Scoped type alias visible while generating a constexpr v3 routine.
    struct scoped_typedef
    {
        /// Type bound to the source-level name.
        type_symbol type;

        RPNX_MEMBER_METADATA(scoped_typedef, type);
    };

    /// Scoped function-local static visible while generating a constexpr v3 routine.
    struct scoped_static
    {
        /// Stable static-local symbol bound to the source-level name.
        static_local_ref symbol;

        RPNX_MEMBER_METADATA(scoped_static, symbol);
    };

    /// Source-level scoped definition visible to constexpr v3 routine generation.
    using scoped_definition_v3 = rpnx::variant< scoped_typedef, scoped_static >;

    /// Result map returned by constexpr v3 evaluation.
    struct constexpr_result_v3
    {
        /// Materialized constexpr results keyed by constexpr_set_result2 result ID.
        std::map< std::uint64_t, constexpr_value > values;
        /// Deduced type for result ID 0 when the caller requested AUTO inference.
        std::optional< type_symbol > deduced_type;

        RPNX_MEMBER_METADATA(constexpr_result_v3, values, deduced_type);
    };

    /// Input for versioned constexpr evaluation.
    struct constexpr_input_v3
    {
        /// Expression to evaluate.
        expression expr;
        /// Context used for source-level name lookup.
        type_symbol context;
        /// Expected expression result type; nullopt discards the expression result.
        std::optional< type_symbol > expected_result_type;
        /// Optional global symbol used when materializing a global antestatal initializer.
        std::optional< type_symbol > antestatal_global_symbol;
        /// Scoped typedef and static bindings visible while generating the routine.
        std::map< std::string, scoped_definition_v3 > scoped_definitions;
        /// Static values visible to the evaluation, keyed by stable static-local symbol.
        std::map< static_local_ref, constexpr_static > statics;

        RPNX_MEMBER_METADATA(constexpr_input_v3, expr, context, expected_result_type, antestatal_global_symbol, scoped_definitions, statics);
    };

    /// Generated constexpr v3 VMIR plus primary-result type deduction metadata.
    struct constexpr_routine_v3_result
    {
        /// Generated VMIR routine to execute in the constexpr interpreter.
        vmir2::functanoid_routine3 routine;
        /// Deduced type for result ID 0 when AUTO result inference was requested.
        std::optional< type_symbol > deduced_type;

        RPNX_MEMBER_METADATA(constexpr_routine_v3_result, routine, deduced_type);
    };

    struct constexpr_input
    {
        expression expr;
        type_symbol context;
        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;
        /// Static objects visible to the constexpr routine, keyed by stable static-local symbol.
        std::map< static_local_ref, constexpr_static > static_inputs;
        /// Source names that should resolve to entries in static_inputs.
        std::map< std::string, static_local_ref > scoped_static_symbols;

        RPNX_MEMBER_METADATA(constexpr_input, expr, context, scoped_definitions, static_inputs, scoped_static_symbols);
    };

    struct constexpr_input2
    {
        expression expr;
        type_symbol context;
        type_symbol type;
        std::optional< type_symbol > antestatal_global_symbol;
        std::map< std::string, rpnx::variant< constexpr_result, type_symbol > > scoped_definitions;
        /// Static objects visible to the constexpr routine, keyed by stable static-local symbol.
        std::map< static_local_ref, constexpr_static > static_inputs;
        /// Source names that should resolve to entries in static_inputs.
        std::map< std::string, static_local_ref > scoped_static_symbols;
        /// True when result ID 0 must be emitted and returned as a primary result.
        bool require_antestatal_result = true;
        /// True when mutable static inputs should be returned using their nonzero result IDs.
        bool emit_static_results = false;

        RPNX_MEMBER_METADATA(constexpr_input2, expr, context, type, antestatal_global_symbol, scoped_definitions, static_inputs, scoped_static_symbols, require_antestatal_result, emit_static_results);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD
