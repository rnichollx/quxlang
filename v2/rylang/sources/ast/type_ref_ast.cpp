//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/ast/type_ref_ast.hpp"

std::string rylang::type_ref_ast::to_string()
{
    return std::visit(
        [](auto&& arg) -> std::string
        {
            return arg.to_string();
        },
        val.get());
}