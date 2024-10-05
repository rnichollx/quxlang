// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_TYPE_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_DATA_TYPE_PLACEMENT_INFO_HEADER_GUARD

#include <cstddef>

#include <rpnx/metadata.hpp>

namespace quxlang
{
    struct type_placement_info
    {
        std::size_t size;
        std::size_t alignment;

        RPNX_MEMBER_METADATA(type_placement_info, size, alignment);
    };
} // namespace quxlang

#endif // QUXLANG_TYPE_PLACEMENT_INFO_HEADER_GUARD
