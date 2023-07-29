//
// Created by Ryan Nicholl on 7/24/23.
//

#ifndef RPNX_RYANSCRIPT1031_NAMESPACE_AST_HEADER
#define RPNX_RYANSCRIPT1031_NAMESPACE_AST_HEADER

#include "rpnx/value.hpp"
#include "rylang/ast/class_ast.hpp"
#include "rylang/ast/function_ast.hpp"

namespace rylang
{
    struct namespace_ast;
    struct function_ast;
    struct class_ast;

    struct namespace_ast
    {
        std::vector< rpnx::value< class_ast > > classes;
        std::vector< rpnx::value< namespace_ast > > namespaces;
        std::vector< rpnx::value< function_ast > > functions;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_NAMESPACE_AST_HEADER
