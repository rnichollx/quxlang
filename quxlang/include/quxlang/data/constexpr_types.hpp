// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD
#define QUXLANG_DATA_CONSTEXPR_TYPES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/dependencies.hpp>
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

    /// Returns the serialoid byte payload stored in a constexpr v3 value without allowing the caller to mutate it.
    inline auto constexpr_value_as_serialoid(constexpr_value const& value) -> constexpr_serialoid const&
    {
        if (!typeis< constexpr_serialoid >(value))
        {
            throw compiler_bug("constexpr value is not a serialoid value");
        }
        return as< constexpr_serialoid >(value);
    }

    /// Returns the mutable serialoid byte payload stored in a constexpr v3 value so the caller can update the bytes in place.
    inline auto constexpr_value_as_serialoid(constexpr_value& value) -> constexpr_serialoid&
    {
        if (!typeis< constexpr_serialoid >(value))
        {
            throw compiler_bug("constexpr value is not a serialoid value");
        }
        return as< constexpr_serialoid >(value);
    }

    /// Returns the string payload stored in a constexpr v3 value without allowing the caller to mutate it.
    inline auto constexpr_value_as_string(constexpr_value const& value) -> constexpr_string const&
    {
        if (!typeis< constexpr_string >(value))
        {
            throw compiler_bug("constexpr value is not a string value");
        }
        return as< constexpr_string >(value);
    }

    /// Returns the mutable string payload stored in a constexpr v3 value so the caller can update the bytes in place.
    inline auto constexpr_value_as_string(constexpr_value& value) -> constexpr_string&
    {
        if (!typeis< constexpr_string >(value))
        {
            throw compiler_bug("constexpr value is not a string value");
        }
        return as< constexpr_string >(value);
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

    /// Result map returned by constexpr v3 evaluation.
    struct constexpr_result_v3
    {
        /// Materialized constexpr results keyed by constexpr_set_result2 result ID.
        std::map< std::uint64_t, constexpr_value > values;
        /// Deduced type for result ID 0 when the caller requested AUTO inference.
        std::optional< type_symbol > deduced_type;
        /// Type symbol produced by a void expression whose result is a type binding.
        std::optional< type_symbol > type_binding_result;

        /// Changed static objects, identified independently of their source names.
        std::map< static_local_ref, constexpr_value > static_updates;
        RPNX_MEMBER_METADATA(constexpr_result_v3, values, deduced_type, type_binding_result, static_updates);
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
        /// Allows the evaluator to return updates to mutable static objects.
        bool mutable_statics = false;
        RPNX_MEMBER_METADATA(constexpr_input_v3, expr, context, expected_result_type, antestatal_global_symbol, mutable_statics);
    };

    /// Generated constexpr v3 VMIR plus primary-result type deduction metadata.
    struct constexpr_routine_v3_result
    {
        /// Generated VMIR routine to execute in the constexpr interpreter.
        vmir2::functanoid_routine3 routine;
        /// Deduced type for result ID 0 when AUTO result inference was requested.
        std::optional< type_symbol > deduced_type;
        /// Type symbol produced by a void expression whose result is a type binding.
        std::optional< type_symbol > type_binding_result;
        /// Direct dependencies of the generated root routine in constexpr mode.
        dependencies direct_dependencies;
        /// Direct dependencies of function-local static values keyed by their stable symbols.
        std::map< static_local_ref, dependencies > static_dependencies;

        RPNX_MEMBER_METADATA(constexpr_routine_v3_result, routine, deduced_type, type_binding_result, direct_dependencies, static_dependencies);
    };

    /// Generated legacy constexpr VMIR and its cached direct dependency inventories.
    struct constexpr_routine_result
    {
        /// Generated VMIR routine to execute in the constexpr interpreter.
        vmir2::functanoid_routine3 routine;
        /// Direct dependencies of the generated root routine in constexpr mode.
        dependencies direct_dependencies;
        /// Direct dependencies of function-local static values keyed by their stable symbols.
        std::map< static_local_ref, dependencies > static_dependencies;

        RPNX_MEMBER_METADATA(constexpr_routine_result, routine, direct_dependencies, static_dependencies);
    };

    /// Constant-expression lookup in a lexical context.
    struct constexpr_input
    {
        expression expr;
        type_symbol context;
        RPNX_MEMBER_METADATA(constexpr_input, expr, context);
    };

    /// Typed constant-expression lookup in a lexical context.
    struct constexpr_input2
    {
        expression expr;
        type_symbol context;
        type_symbol type;
        std::optional< type_symbol > antestatal_global_symbol;
        bool require_antestatal_result = true;
        bool emit_static_results = false;
        RPNX_MEMBER_METADATA(constexpr_input2, expr, context, type, antestatal_global_symbol, require_antestatal_result, emit_static_results);
    };
}
#endif
