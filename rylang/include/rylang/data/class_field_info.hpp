//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER

#include "canonical_type_reference.hpp"
#include <vector>
#include "rylang/data/qualified_reference.hpp"


namespace rylang

{
    struct class_field_info
    {
        std::string name;
        qualified_symbol_reference type;
        std::size_t offset = 0;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER
