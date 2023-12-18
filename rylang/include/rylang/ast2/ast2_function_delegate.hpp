//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST_2_FUNCTION_DECLARATION_HEADER_GUARD
#define AST_2_FUNCTION_DECLARATION_HEADER_GUARD

#include <rylang/ast/function_arg_ast.hpp>
#include <rylang/ast2/ast2_function_arg.hpp>
#include <rylang/data/function_block.hpp>
#include <rylang/data/function_delegate.hpp>

namespace rylang
{
    struct ast2_function_delegate
    {
        type_symbol target;
        std::vector< expression > args;
        std::strong_ordering operator<=>(const ast2_function_delegate& other) const = default;
    };


} // namespace rylang

#endif // AST_2_FUNCTION_DECLARATION_HEADER_GUARD
