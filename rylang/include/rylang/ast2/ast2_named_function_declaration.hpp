//
// Created by Ryan Nicholl on 12/12/23.
//

#ifndef AST2_CLASS_FUNCTION_DECLARATION_HPP
#define AST2_CLASS_FUNCTION_DECLARATION_HPP


#include <string>
#include <rylang/ast2/ast2_function_declaration.hpp>

namespace rylang
{
    struct ast2_named_function_declaration
    {
        std::string name;
        bool is_field = false;
        ast2_function_declaration function;
    };

} // namespace rylang

#endif // AST2_CLASS_FUNCTION_DECLARATION_HPP
