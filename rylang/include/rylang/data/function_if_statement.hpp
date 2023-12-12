//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef RYLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD
#define RYLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD

#include "expression.hpp"

#include "rylang/data/function_block.hpp"

namespace rylang
{
    struct function_if_statement
    {
        expression condition;
        function_block then_block;
        std::optional< function_block > else_block;

        std::strong_ordering operator<=>(const function_if_statement& other) const = default;
    };

} // namespace rylang

#endif // RYLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD
