//
// Created by Ryan Nicholl on 11/14/23.
//

#ifndef RYLANG_FUNCTION_WHILE_STATEMENT_HEADER_GUARD
#define RYLANG_FUNCTION_WHILE_STATEMENT_HEADER_GUARD


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

#endif // RYLANG_FUNCTION_WHILE_STATEMENT_HEADER_GUARD
