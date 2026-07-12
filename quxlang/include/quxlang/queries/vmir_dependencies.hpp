// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD
#define QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD

#include <quxlang/data/dependencies.hpp>
#include <quxlang/data/functanoid_requirements.hpp>

namespace quxlang
{
    /** Selects one symbol and execution mode for direct dependency scanning. */
    struct direct_dependencies_input
    {
        type_symbol symbol;
        dependency_set set = dependency_set::native;

        RPNX_MEMBER_METADATA(direct_dependencies_input, symbol, set);
    };

    struct direct_dependencies_query
    {
        static constexpr auto query_id = "direct_dependencies";
        using input_type = direct_dependencies_input;
        using output_type = dependencies;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD
