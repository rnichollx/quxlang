//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_LVALUE_REFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_LVALUE_REFERENCE_HEADER

#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    struct expression_lvalue_reference
    {
        lookup_chain chain;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_LVALUE_REFERENCE_HEADER
