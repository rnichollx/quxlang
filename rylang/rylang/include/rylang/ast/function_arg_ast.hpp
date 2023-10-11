//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER

#include <string>

#include "type_ref_ast.hpp"

namespace rylang
{
    struct function_arg_ast
    {
        std::string external_name;
        std::string name;
        type_ref_ast type;
        std::string to_string()
        {
            return "ast_arg{ external_name: " + external_name + ", name: " + name + ", type: " + type.to_string() + " }";
        }
        bool operator<(function_arg_ast const&) const;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER
