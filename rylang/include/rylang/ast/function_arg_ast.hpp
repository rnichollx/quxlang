//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_FUNCTION_ARG_AST_HEADER_GUARD
#define RYLANG_FUNCTION_ARG_AST_HEADER_GUARD

#include <string>

#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    struct function_arg_ast
    {
        std::string external_name;
        std::string name;
        type_symbol type;

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

#endif // RYLANG_FUNCTION_ARG_AST_HEADER_GUARD
