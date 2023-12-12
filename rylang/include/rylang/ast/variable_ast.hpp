//
// Created by Ryan Nicholl on 7/25/23.
//

#ifndef RYLANG_VARIABLE_AST_HEADER_GUARD
#define RYLANG_VARIABLE_AST_HEADER_GUARD

//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
#define RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD

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

#endif // RYLANG_MEMBER_VARIABLE_AST_HEADER_GUARD


#endif // RYLANG_VARIABLE_AST_HEADER_GUARD
