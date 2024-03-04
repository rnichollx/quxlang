//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_FUNCTION_ARG_AST_HEADER_GUARD
#define QUXLANG_FUNCTION_ARG_AST_HEADER_GUARD

#include <string>

#include "quxlang/data/qualified_symbol_reference.hpp"

namespace quxlang
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
        bool operator<(function_arg_ast const& other) const
        {
            return std::tie(external_name, name, type) < std::tie(other.external_name, other.name, other.type);
        }
        auto operator<=>(function_arg_ast const & other) const
        {
            return std::tie(external_name, name, type) <=> std::tie(other.external_name, other.name, other.type);
        }
    };

} // namespace quxlang

#endif // QUXLANG_FUNCTION_ARG_AST_HEADER_GUARD
