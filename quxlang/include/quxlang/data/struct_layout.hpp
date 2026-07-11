// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD
#define QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD

#include "struct_field_info.hpp"
#include <vector>

namespace quxlang
{
    /** Describes the fields, size, and alignment of a struct class. */
    struct struct_layout
    {
        std::vector<struct_field_info> fields;
        std::uint64_t size = 0;
        std::uint64_t align = 0;

        RPNX_MEMBER_METADATA(struct_layout, fields, size, align);
    };
} // namespace quxlang

#endif // QUXLANG_DATA_STRUCT_LAYOUT_HEADER_GUARD
