//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER

#include <string>

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct function_arg_ast
    {
        std::string external_name;
        std::string name;
        qualified_symbol_reference type;
        std::string to_string()
        {

            return "arg";
            //   return "ast_arg{ external_name: " + external_name + ", name: " + name + ", type: " + type.to_string() + " }";
        }
        bool operator<(function_arg_ast const&) const
        {
            return std::tie(external_name, name, type) < std::tie(external_name, name, type);
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_ARG_AST_HEADER
