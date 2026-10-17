// Copyright 2024-2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD

#include <cstddef>
#include <quxlang/manipulators/struct_math.hpp>

#include <rpnx/macros.hpp>

namespace quxlang
{
    /** Describes the size and alignment of a class object. */
    struct class_placement_info
    {
        std::uint64_t size;
        std::uint64_t alignment;

        /** Returns the spacing between adjacent objects, including alignment padding. */
        auto cell_size(std::uint64_t maximum = std::numeric_limits< std::uint64_t >::max()) const -> std::uint64_t
        {
            std::uint64_t result = size;
            advance_to_alignment(result, alignment, maximum);
            return result;
        }

        RPNX_MEMBER_METADATA(class_placement_info, size, alignment);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_CLASS_PLACEMENT_INFO_HEADER_GUARD
