//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_AST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_AST_HEADER

#include "member_variable_ast.hpp"
#include <vector>

namespace rylang
{
    struct class_ast
    {
        class_ast() = delete;
        std::vector< member_variable_ast > member_variables;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_AST_HEADER
