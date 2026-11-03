// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_LIFETIME_SNAPSHOTS_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_LIFETIME_SNAPSHOTS_HEADER_GUARD

#include <quxlang/vmir2/coroutine_vmir_analysis.hpp>

#include <cstddef>
#include <vector>

namespace quxlang::vmir2
{
    /** Associates a shared lifetime root with one logical CFG boundary. */
    struct coroutine_snapshot_observation
    {
        state_map::snapshot root;
        std::size_t node = 0;
        std::size_t instruction = 0;
    };

    /** Counts represented snapshot and metadata work independently of expanded live sets. */
    struct coroutine_snapshot_statistics
    {
        std::size_t roots = 0;
        std::size_t nodes = 0;
        std::size_t slot_versions = 0;
        std::size_t metadata_payloads = 0;
        std::size_t metadata_operands = 0;
    };

    /** Adds conservative hidden obligations by traversing shared lifetime DAGs once. */
    auto record_coroutine_snapshot_lifetimes(functanoid_routine3 const& routine, std::vector< coroutine_snapshot_observation > const& observations,
                                           coroutine_vmir_analysis& output) -> coroutine_snapshot_statistics;
}

#endif // QUXLANG_VMIR2_COROUTINE_LIFETIME_SNAPSHOTS_HEADER_GUARD
