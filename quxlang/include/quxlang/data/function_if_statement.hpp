//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD
#define QUXLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD

#include "expression.hpp"

#include "quxlang/data/function_block.hpp"

namespace quxlang
{
    struct function_if_statement
    {
        expression condition;
        function_block then_block;
        std::optional< function_block > else_block;

        RPNX_MEMBER_METADATA(function_if_statement, condition, then_block, else_block);
    };

} // namespace quxlang

#endif // QUXLANG_FUNCTION_IF_STATEMENT_HEADER_GUARD