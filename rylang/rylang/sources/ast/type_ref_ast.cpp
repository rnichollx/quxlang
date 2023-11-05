//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/ast/type_ref_ast.hpp"
#include "rylang/ast/pointer_ref_ast.hpp"



bool rylang::type_ref_ast::operator<(type_ref_ast const& other) const
{
    return val < other.val;
}

