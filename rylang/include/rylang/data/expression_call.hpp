//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_EXPRESSION_CALL_HEADER_GUARD
#define RYLANG_EXPRESSION_CALL_HEADER_GUARD

#include "rylang/data/expression.hpp"
#include <vector>

namespace rylang
{
    struct expression_call
    {
        expression callee;
        std::vector< expression > args;

        std::strong_ordering operator<=>(const expression_call& other) const = default;
    };
} // namespace rylang

#endif // RYLANG_EXPRESSION_CALL_HEADER_GUARD
