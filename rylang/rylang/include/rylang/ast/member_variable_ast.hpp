//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER

#include "type_ref_ast.hpp"
#include <string>

namespace rylang
{
    struct member_variable_ast
    {
        std::string name;
        type_ref_ast type;

        std::string to_string()
        {
            return "ast_member_variable{ name: " + name + ", type: " + type.to_string() + " }";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
