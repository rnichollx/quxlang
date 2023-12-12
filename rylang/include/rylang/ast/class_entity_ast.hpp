//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RYLANG_CLASS_ENTITY_AST_HEADER_GUARD
#define RYLANG_CLASS_ENTITY_AST_HEADER_GUARD

#include <string>
#include <compare>
#include <set>

namespace rylang
{
    struct entity_ast;
    struct class_entity_ast
    {
        std::set< std::string > m_keywords;
        std::vector<std::string> m_var_order;
        std::string to_string(entity_ast const*) const;
        std::strong_ordering operator<=>(class_entity_ast const&) const = default;
    };

} // namespace rylang

#endif // RYLANG_CLASS_ENTITY_AST_HEADER_GUARD
