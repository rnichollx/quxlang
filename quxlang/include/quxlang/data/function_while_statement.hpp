// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_WHILE_STATEMENT_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_WHILE_STATEMENT_HEADER_GUARD


#include "expression.hpp"
#include <compare>
#include "quxlang/data/function_block.hpp"


namespace quxlang
{
    struct function_while_statement
    {
        expression condition;
        function_block loop_block;

        RPNX_MEMBER_METADATA(function_while_statement, condition, loop_block);
    };

} // namespace quxlang

#endif // QUXLANG_FUNCTION_WHILE_STATEMENT_HEADER_GUARD
