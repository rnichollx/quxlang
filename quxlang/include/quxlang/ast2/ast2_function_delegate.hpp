// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_AST2_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_AST2_AST2_FUNCTION_DELEGATE_HEADER_GUARD

#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/data/function_block.hpp>
#include <quxlang/data/function_delegate.hpp>

namespace quxlang
{
    struct ast2_function_delegate
    {
        type_symbol target;
        std::vector< expression > args;


        RPNX_MEMBER_METADATA(ast2_function_delegate, target, args)
    };


} // namespace quxlang

#endif // AST_2_FUNCTION_DECLARATION_HEADER_GUARD
