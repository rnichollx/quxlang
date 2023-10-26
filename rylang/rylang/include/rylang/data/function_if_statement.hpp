//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_IF_STATEMENT_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_IF_STATEMENT_HEADER

#include "expression.hpp"

#include "rylang/data/function_block.hpp"

namespace rylang
{
    struct function_if_statement
    {
        expression condition;
        function_block then_block;
        std::optional< function_block > else_block;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_IF_STATEMENT_HEADER
