// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_DEPENDENCIES_HEADER_GUARD
#define QUXLANG_DATA_DEPENDENCIES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <cstdint>
#include <map>
#include <optional>
#include <set>

// clang-format off
RPNX_ENUM(quxlang, vmir_runtime_dependency, std::uint8_t,
    assert_fail,
    panic,
    initguard_complete,
    initguard_abort,
    initguard_try_acquire
)
// clang-format on

namespace quxlang
{
    /** Complete direct dependency inventory for one dependency source. */
    struct dependencies
    {
        std::map< type_symbol, std::optional< source_location > > functanoids;
        std::set< type_symbol > antestatal_globals;
        std::set< type_symbol > global_roots;
        std::set< type_symbol > type_placements;
        std::set< type_symbol > struct_layouts;
        std::set< type_symbol > fusion_layouts;
        std::set< static_snapshot_ref > static_snapshots;
        std::set< vmir_runtime_dependency > runtime_dependencies;

        RPNX_MEMBER_METADATA(dependencies, functanoids, antestatal_globals, global_roots, type_placements, struct_layouts, fusion_layouts, static_snapshots, runtime_dependencies);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_DEPENDENCIES_HEADER_GUARD
