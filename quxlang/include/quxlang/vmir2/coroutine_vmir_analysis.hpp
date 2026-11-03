// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_VMIR2_COROUTINE_VMIR_ANALYSIS_HEADER_GUARD
#define QUXLANG_VMIR2_COROUTINE_VMIR_ANALYSIS_HEADER_GUARD

#include <quxlang/vmir2/coroutine_frame_analysis.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <set>

namespace quxlang::vmir2
{
    /** Carries the compact CFG adapter and logical aliases reconstructed from persistent owners. */
    struct coroutine_vmir_analysis
    {
        coroutine_frame_input input;
        std::set< local_index > borrowed_locals;
        std::map< local_index, type_symbol > borrowed_parameters;
        std::set< local_index > saved_array_element_addresses;
        /// Captured alias addresses used by detached cleanup ownership.
        std::set< local_index > saved_cleanup_alias_addresses;
        /// Counts shared state and array-membership nodes visited by lifetime analysis.
        std::size_t snapshot_nodes = 0;
        /// Counts hidden local obligations after shared payload ranges are joined.
        std::size_t snapshot_operands = 0;
        /// Counts represented alias operands, edges, and backward-propagation visits.
        std::size_t alias_work = 0;
    };

    /** Builds bounded lifetime evidence from the logical CFG, shared slot snapshots, and operands. */
    [[nodiscard]] auto coroutine_analysis_input(functanoid_routine3 const& routine) -> coroutine_vmir_analysis;

    /** Returns the distinct logical operands of an instruction for bounded lifetime-transition observation. */
    [[nodiscard]] auto coroutine_instruction_locals(vm_instruction const& instruction) -> std::vector< local_index >;

    /** Includes represented subobject bindings retired by aggregate lifetime instructions. */
    [[nodiscard]] auto coroutine_instruction_locals(vm_instruction const& instruction, state_map const& state) -> std::vector< local_index >;
}

#endif // QUXLANG_VMIR2_COROUTINE_VMIR_ANALYSIS_HEADER_GUARD
