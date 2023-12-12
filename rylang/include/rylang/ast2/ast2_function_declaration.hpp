//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST_2_FUNCTION_DECLARATION_HEADER_GUARD
#define AST_2_FUNCTION_DECLARATION_HEADER_GUARD

#include <rylang/ast/function_arg_ast.hpp>
#include <rylang/data/function_block.hpp>
#include <rylang/data/function_delegate.hpp>

namespace rylang
{

    class ast2_function_declaration
    {
        std::vector< function_arg_ast > args;
        std::optional< type_symbol > return_type;
        std::optional< type_symbol > this_type;
        std::vector< function_delegate > delegates;
        std::optional< std::int64_t > priority;
        function_block body;
    };

} // namespace rylang

#endif // AST_2_FUNCTION_DECLARATION_HEADER_GUARD
