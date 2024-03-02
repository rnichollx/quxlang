//
// Created by Ryan Nicholl on 7/25/23.
//

#ifndef QUXLANG_VARIABLE_AST_HEADER_GUARD
#define QUXLANG_VARIABLE_AST_HEADER_GUARD

//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD
#define QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD

#include "type_ref_ast.hpp"
#include <string>

namespace quxlang
{
    struct variable_ast
    {
        type_ref_ast type;

        std::string to_string()
        {
            return "ast_variable{ type: " + type.to_string() + " }";
        }
    };
} // namespace quxlang

#endif // QUXLANG_MEMBER_VARIABLE_AST_HEADER_GUARD


#endif // QUXLANG_VARIABLE_AST_HEADER_GUARD
