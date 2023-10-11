//
// Created by Ryan Nicholl on 7/25/23.
//
#include "rylang/ast/entity_ast.hpp"

std::string rylang::entity_ast::to_string() const
{
    auto result = std::string("{ENTITY, sub-entities: ");

    for (auto const& i : m_sub_entities)
    {
        result += " " + i.first + ": " + i.second.get().to_string() + ", ";
    }
    auto substr = std::visit(
        [this](auto&& arg) -> std::string
        {
            return arg.to_string(this);
        },
        m_subvalue.get());



    result += "subvalue: ";
    result += substr;

    result += " }";
    return result;
}
