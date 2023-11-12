//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_LAYOUT_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_LAYOUT_HEADER

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

#endif // RPNX_RYANSCRIPT1031_CLASS_LAYOUT_HEADER
