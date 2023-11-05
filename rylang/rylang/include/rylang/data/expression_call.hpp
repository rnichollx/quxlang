//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_CALL_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_CALL_HEADER

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

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_CALL_HEADER
