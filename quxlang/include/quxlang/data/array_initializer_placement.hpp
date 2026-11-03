// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_ARRAY_INITIALIZER_PLACEMENT_HEADER_GUARD
#define QUXLANG_DATA_ARRAY_INITIALIZER_PLACEMENT_HEADER_GUARD

#include <quxlang/data/class_placement_info.hpp>
#include <quxlang/data/machine.hpp>
#include <quxlang/exception.hpp>
#include <quxlang/manipulators/struct_math.hpp>

#include <algorithm>

namespace quxlang
{
    /** Returns the target placement of the array-initializer pointer, count, and progress record. */
    inline auto array_initializer_placement(machine_target_info const& machine) -> class_placement_info
    {
        std::uint64_t integer_alignment = machine.integer_alignment_for_bits(64);
        std::uint64_t alignment = std::max(machine.pointer_align(), integer_alignment);
        std::uint64_t extent = machine.pointer_size_bytes();
        std::uint64_t maximum = machine.maximum_object_extent();
        advance_to_alignment(extent, integer_alignment, maximum);
        extent = add_object_extent(extent, 8, maximum);
        advance_to_alignment(extent, integer_alignment, maximum);
        extent = add_object_extent(extent, 8, maximum);
        advance_to_alignment(extent, alignment, maximum);
        return {.size = extent, .alignment = alignment};
    }
}

#endif // QUXLANG_DATA_ARRAY_INITIALIZER_PLACEMENT_HEADER_GUARD
