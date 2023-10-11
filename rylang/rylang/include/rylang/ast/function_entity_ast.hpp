//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_FUNCTION_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_FUNCTION_ENTITY_AST_HEADER

#include "rylang/ast/entity_ast.hpp"
#include <string>
namespace rylang
{
    struct function_entity_ast
    {
        std::vector< function_ast > m_function_overloads;

        std::string to_string(entity_ast const *) const {
                return "function_ast{ <to_string not implemented> }";
                }
        bool operator < (function_entity_ast const& other) const
        {
            return m_function_overloads < other.m_function_overloads;
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_FUNCTION_ENTITY_AST_HEADER
