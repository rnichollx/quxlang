// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD
#define QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD

#include <quxlang/data/functanoid_requirements.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <map>
#include <optional>
#include <set>

// clang-format off
RPNX_ENUM(quxlang, vmir_runtime_dependency, std::uint8_t,
    assert_fail,
    initguard_complete,
    initguard_abort,
    initguard_try_acquire
)
// clang-format on

namespace quxlang
{
    /** Selects one symbol and execution mode for direct dependency scanning. */
    struct direct_dependencies_input
    {
        type_symbol symbol;
        dependency_set set = dependency_set::native;

        RPNX_MEMBER_METADATA(direct_dependencies_input, symbol, set);
    };

    /** Complete direct dependency inventory for reachable blocks of one VMIR routine. */
    struct dependencies
    {
        std::map< type_symbol, std::optional< source_location > > functanoids;
        std::set< type_symbol > antestatal_globals;
        std::set< type_symbol > global_roots;
        std::set< type_symbol > type_placements;
        std::set< type_symbol > struct_layouts;
        std::set< vmir_runtime_dependency > runtime_dependencies;

        RPNX_MEMBER_METADATA(dependencies, functanoids, antestatal_globals, global_roots, type_placements, struct_layouts, runtime_dependencies);
    };

    struct direct_dependencies_query
    {
        static constexpr auto query_id = "direct_dependencies";
        using input_type = direct_dependencies_input;
        using output_type = dependencies;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_VMIR_DEPENDENCIES_HEADER_GUARD
