// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_MANIPULATORS_STRUCT_MATH_HEADER_GUARD
#define QUXLANG_MANIPULATORS_STRUCT_MATH_HEADER_GUARD

#include <cstdint>
#include <limits>
#include <quxlang/exception.hpp>

namespace quxlang
{
    /** Adds object extents without exceeding the selected representation limit. */
    inline auto add_object_extent(std::uint64_t offset, std::uint64_t size, std::uint64_t maximum) -> std::uint64_t
    {
        if (offset > maximum || size > maximum - offset)
        {
            throw semantic_compilation_error("Object extent exceeds the target size limit");
        }
        return offset + size;
    }

    /** Multiplies an element extent by its count without overflowing the target size representation. */
    inline auto multiply_object_extent(std::uint64_t size, std::uint64_t count, std::uint64_t maximum) -> std::uint64_t
    {
        if (size > maximum || (size != 0 && count > maximum / size))
        {
            throw semantic_compilation_error("Array extent exceeds the target size limit");
        }
        return size * count;
    }

    /** Advances an object offset to its alignment without overflowing the selected size representation. */
    inline void advance_to_alignment(std::uint64_t& offset, std::uint64_t alignment,
                                     std::uint64_t maximum = std::numeric_limits< std::uint64_t >::max())
    {
        if (offset > maximum || alignment > maximum)
        {
            throw semantic_compilation_error("Object alignment or extent exceeds the target size limit");
        }
        if (alignment <= 1) return;
        std::uint64_t remainder = offset % alignment;
        if (remainder != 0)
        {
            offset = add_object_extent(offset, alignment - remainder, maximum);
        }
    }

}

#endif // QUXLANG_STRUCT_MATH_HEADER_GUARD
