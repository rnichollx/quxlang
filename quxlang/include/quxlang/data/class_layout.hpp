//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef QUXLANG_CLASS_LAYOUT_HEADER_GUARD
#define QUXLANG_CLASS_LAYOUT_HEADER_GUARD

#include "class_field_info.hpp"
#include <vector>

namespace quxlang
{
    struct class_layout
    {
        std::vector<class_field_info> fields;
        std::size_t size = 0;
        std::size_t align = 0;

        RPNX_MEMBER_METADATA(class_layout, fields, size, align);
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_LAYOUT_HEADER_GUARD
