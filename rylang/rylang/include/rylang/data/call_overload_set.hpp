//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER
#define RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER

#include "rylang/data/canonical_type_reference.hpp"

namespace rylang
{
    struct call_overload_set
    {
        std::vector< canonical_type_reference > argument_types;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CALL_OVERLOAD_SET_HEADER
