//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_S_EXPRESSION_CALL_HEADER
#define RPNX_RYANSCRIPT1031_S_EXPRESSION_CALL_HEADER

#include "rylang/data/s_expression.hpp"
#include <memory>
#include <vector>

namespace rylang
{
    struct s_expression_call
    {
        s_expression_ptr callee;
        std::vector< s_expression_ptr > args;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_S_EXPRESSION_CALL_HEADER
