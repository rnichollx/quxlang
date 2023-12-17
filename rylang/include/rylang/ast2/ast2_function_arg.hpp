//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef AST2_FUNCTION_ARG_HPP
#define AST2_FUNCTION_ARG_HPP
#include <rylang/data/qualified_symbol_reference.hpp>

namespace rylang
{
    struct ast2_function_arg
    {
        std::string name;
        std::optional<std::string> api_name;
        type_symbol type;

        std::strong_ordering operator<=>(const ast2_function_arg& other) const = default;
    };
}


#endif //AST2_FUNCTION_ARG_HPP
