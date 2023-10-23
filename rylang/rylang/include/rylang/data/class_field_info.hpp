//
// Created by Ryan Nicholl on 10/22/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER

#include "canonical_lookup_chain.hpp"
#include <vector>

namespace rylang

{
    struct class_field_info
    {
        canonical_lookup_chain type;
        std::size_t offset = 0;
        std::size_t size = 0;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_FIELD_INFO_HEADER
