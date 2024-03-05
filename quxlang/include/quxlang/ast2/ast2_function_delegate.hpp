//
// Created by Ryan Nicholl on 12/11/23.
//

#ifndef AST_2_FUNCTION_DECLARATION_HEADER_GUARD
#define AST_2_FUNCTION_DECLARATION_HEADER_GUARD

#include <quxlang/ast/function_arg_ast.hpp>
#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/data/function_block.hpp>
#include <quxlang/data/function_delegate.hpp>

namespace quxlang
{
    struct ast2_function_delegate
    {
        type_symbol target;
        std::vector< expression > args;
        auto  operator<=>(const ast2_function_delegate& other) const
        {
            return rpnx::compare(target, other.target, args, other.args);
        }
    };


} // namespace quxlang

#endif // AST_2_FUNCTION_DECLARATION_HEADER_GUARD
