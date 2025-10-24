// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_CLASS_LAYOUT_HEADER_GUARD
#define QUXLANG_DATA_CLASS_LAYOUT_HEADER_GUARD

#include "class_field_info.hpp"
#include <vector>

namespace quxlang
{
    struct class_layout
    {
        std::vector<class_field_info> fields;
        std::uint64_t size = 0;
        std::uint64_t align = 0;

        RPNX_MEMBER_METADATA(class_layout, fields, size, align);
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_LAYOUT_HEADER_GUARD
