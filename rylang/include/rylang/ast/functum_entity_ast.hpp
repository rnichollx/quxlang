//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RYLANG_FUNCTUM_ENTITY_AST_HEADER_GUARD
#define RYLANG_FUNCTUM_ENTITY_AST_HEADER_GUARD

#include "rylang/ast/entity_ast.hpp"
#include <string>
namespace rylang
{
    struct functum_entity_ast
    {
        std::vector< function_ast > m_function_overloads;

        std::string to_string(entity_ast const*) const
        {
            return "functum_ast{ <to_string not implemented> }";
        }
        bool operator<(functum_entity_ast const& other) const
        {
            return m_function_overloads < other.m_function_overloads;
        }
    };

} // namespace rylang

#endif // RYLANG_FUNCTUM_ENTITY_AST_HEADER_GUARD
