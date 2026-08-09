// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INCLUDE_IF_IS_ACTIVE_HEADER_GUARD
#define QUXLANG_QUERIES_INCLUDE_IF_IS_ACTIVE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <optional>

namespace quxlang
{
    /** Identifies an optional INCLUDE_IF condition and its evaluation scope. */
    struct include_if_is_active_input
    {
        /// Canonical scope in which the condition is evaluated.
        type_symbol context;
        /// Conditional expression, or no value for an unconditional declaration.
        std::optional< expression > condition;

        RPNX_MEMBER_METADATA(include_if_is_active_input, context, condition);
    };

    /** Reports whether an optional INCLUDE_IF condition includes its declaration. */
    struct include_if_is_active_query
    {
        static constexpr auto query_id = "include_if_is_active";
        using input_type = include_if_is_active_input;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INCLUDE_IF_IS_ACTIVE_HEADER_GUARD
