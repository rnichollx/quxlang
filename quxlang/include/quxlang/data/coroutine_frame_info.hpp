// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_COROUTINE_FRAME_INFO_HEADER_GUARD
#define QUXLANG_DATA_COROUTINE_FRAME_INFO_HEADER_GUARD

#include <quxlang/data/struct_field_declaration.hpp>
#include <quxlang/data/struct_field_info.hpp>
#include <quxlang/vmir2/vmir2.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

namespace quxlang
{
    /** Describes one nominal frame's semantic fields, physical overlays, and retained logical slots. */
    struct coroutine_frame_info
    {
        std::vector< struct_field > fields;
        std::vector< struct_field_info > layout_fields;
        std::map< vmir2::local_index, std::size_t > local_fields;
        std::map< vmir2::local_index, std::uint64_t > local_offsets;
        /// Aliases reconstructed from persistent owner and continuation lifetime metadata.
        std::set< vmir2::local_index > borrowed_locals;
        /// Original NVALUE/DESTROY formal types whose fields retain borrowed object addresses.
        std::map< vmir2::local_index, type_symbol > borrowed_parameters;
        /// Completed construction aliases whose addresses outlive the array initializer's current prefix.
        std::set< vmir2::local_index > saved_array_element_addresses;
        /// Captured alias addresses used by detached cleanup ownership.
        std::set< vmir2::local_index > saved_cleanup_alias_addresses;
        std::uint64_t size = 0;
        std::uint64_t alignment = 1;
        std::uint64_t baseline_size = 0;
        std::uint64_t analysis_input_size = 0;
        std::uint64_t analysis_snapshot_nodes = 0;
        std::uint64_t analysis_snapshot_operands = 0;
        std::uint64_t analysis_alias_work = 0;
        std::uint64_t refinement_limit = 0;
        std::uint64_t refinement_work = 0;
        bool refinement_exhausted = false;
        bool refined_layout_selected = false;

        RPNX_MEMBER_METADATA(coroutine_frame_info, fields, layout_fields, local_fields, local_offsets, borrowed_locals, borrowed_parameters, saved_array_element_addresses, saved_cleanup_alias_addresses, size, alignment,
            baseline_size, analysis_input_size, analysis_snapshot_nodes, analysis_snapshot_operands, analysis_alias_work, refinement_limit, refinement_work, refinement_exhausted, refined_layout_selected);
    };
}

#endif // QUXLANG_DATA_COROUTINE_FRAME_INFO_HEADER_GUARD
