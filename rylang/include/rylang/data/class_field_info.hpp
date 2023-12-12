//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RYLANG_CLASS_FIELD_INFO_HEADER_GUARD
#define RYLANG_CLASS_FIELD_INFO_HEADER_GUARD

#include "canonical_type_reference.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include <vector>

namespace rylang

{
    struct class_field_info
    {
        std::string name;
        type_symbol type;
        std::size_t offset = 0;
    };
} // namespace rylang

#endif // RYLANG_CLASS_FIELD_INFO_HEADER_GUARD
