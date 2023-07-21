//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_TYPE_REF_AST_HEADER
#define RPNX_RYANSCRIPT1031_TYPE_REF_AST_HEADER

#include <string>

#include "rpnx/value.hpp"
#include "rylang/ast/null_object_ast.hpp"
#include "rylang/ast/symbol_ref_ast.hpp"
#include "rylang/ast/array_ref_ast.hpp"
#include "rylang/ast/integral_keyword_ast.hpp"

namespace rylang
{
    struct pointer_ref_ast;
    struct type_ref_ast
    {
       rpnx::value< std::variant< null_object_ast, symbol_ref_ast, array_ref_ast, pointer_ref_ast, integral_keyword_ast > > val;
       std::string to_string();
    };
} // namespace rylang

#include "rylang/ast/pointer_ref_ast.hpp"
#endif // RPNX_RYANSCRIPT1031_TYPE_REF_AST_HEADER
