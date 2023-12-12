//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef RYLANG_AST2_AST2_CLASS_HEADER_GUARD
#define RYLANG_AST2_AST2_CLASS_HEADER_GUARD

#include <compare>
#include <rylang/ast2/ast2_class_function_declaration.hpp>
#include <rylang/ast2/ast2_class_variable_declaration.hpp>
#include <rylang/ast2/ast2_function_declaration.hpp>

namespace rylang
{
    struct ast2_class
    {
        std::vector<ast2_class_variable_declaration> member_variables;
        std::vector<ast2_class_variable_declaration> static_variables;

        std::vector<ast2_class_function_declaration> member_functions;



    };
}

#endif //AST2_CLASS_HEADER_GUARD
