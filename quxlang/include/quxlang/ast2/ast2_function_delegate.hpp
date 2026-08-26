// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_AST2_AST2_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_AST2_AST2_FUNCTION_DELEGATE_HEADER_GUARD

#include <quxlang/ast2/ast2_function_arg.hpp>
#include <quxlang/data/function_block.hpp>
#include <quxlang/data/function_delegate.hpp>
#include <quxlang/macros.hpp>

namespace quxlang
{
    struct ast2_function_delegate
    {
        /// Semantic category of the selected constructor target.
        function_delegate_kind kind = function_delegate_kind::ordinary;
        type_symbol target;
        std::vector< expression_arg > args;


        QUXLANG_WITH_SOURCE_LOCATION_METADATA(ast2_function_delegate, kind, target, args)
    };


} // namespace quxlang

#endif // AST_2_FUNCTION_DECLARATION_HEADER_GUARD
