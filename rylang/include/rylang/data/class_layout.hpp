//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RYLANG_CLASS_LAYOUT_HEADER_GUARD
#define RYLANG_CLASS_LAYOUT_HEADER_GUARD

#include "class_field_info.hpp"
#include <vector>

namespace rylang
{
    struct class_layout
    {
        std::vector<class_field_info> fields;
        std::size_t size = 0;
        std::size_t align = 0;
    };
} // namespace rylang

#endif // RYLANG_CLASS_LAYOUT_HEADER_GUARD
