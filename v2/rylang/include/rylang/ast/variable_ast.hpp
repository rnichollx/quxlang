//
// Created by Ryan Nicholl on 7/25/23.
//

#ifndef RPNX_RYANSCRIPT1031_VARIABLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_VARIABLE_AST_HEADER

//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER
#define RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER

#include "type_ref_ast.hpp"
#include <string>

namespace rylang
{
    struct variable_ast
    {
        type_ref_ast type;

        std::string to_string()
        {
            return "ast_variable{ type: " + type.to_string() + " }";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_MEMBER_VARIABLE_AST_HEADER


#endif // RPNX_RYANSCRIPT1031_VARIABLE_AST_HEADER
