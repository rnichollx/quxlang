//
// Created by Ryan Nicholl on 7/25/23.
//
#include "rylang/ast/entity_ast.hpp"

std::string rylang::entity_ast::to_string() const
{
    std::string result = "ast_";

    if (m_is_field_entity)
    {
        result += "member_";
    }
    if (m_category == entity_category::class_cat)
    {
       result += "class";
    }
    else if (m_category == entity_category::function_cat)
    {
        result += "function";
    }
    else if (m_category == entity_category::namespace_cat)
    {
        result += "namespace";
    }
    else if (m_category == entity_category::variable_cat)
    {
        result += "variable";
    }

    else throw std::runtime_error("Unknown entity category");

    result += "{";

    for (auto & x: m_sub_entities)
    {
        result += x.first + ": " + x.second.get().to_string() + ", ";
    }

    if (m_variable_type)
    {
        result += "type: " + m_variable_type.value().to_string() + ", ";
    }

    result += "}";

    return result;
}
