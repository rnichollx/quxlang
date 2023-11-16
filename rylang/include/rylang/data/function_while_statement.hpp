//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_WHILE_STATEMENT_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_WHILE_STATEMENT_HEADER


#include "expression.hpp"
#include <compare>
#include "rylang/data/function_block.hpp"


namespace rylang
{
    struct function_while_statement
    {
        expression condition;
        function_block loop_block;

        std::strong_ordering operator<=>(const function_while_statement& other) const = default;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_WHILE_STATEMENT_HEADER
