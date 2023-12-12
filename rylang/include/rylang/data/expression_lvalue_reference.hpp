//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD
#define RYLANG_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD

#include "rylang/data/lookup_chain.hpp"

namespace rylang
{
    struct expression_lvalue_reference
    {
        lookup_chain chain;
    };
} // namespace rylang

#endif // RYLANG_EXPRESSION_LVALUE_REFERENCE_HEADER_GUARD
