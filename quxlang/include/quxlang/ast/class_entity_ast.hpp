//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef QUXLANG_CLASS_ENTITY_AST_HEADER_GUARD
#define QUXLANG_CLASS_ENTITY_AST_HEADER_GUARD

#include <string>
#include <compare>
#include <set>

namespace quxlang
{
    struct entity_ast;
    struct class_entity_ast
    {
        std::set< std::string > m_keywords;
        std::vector<std::string> m_var_order;
        std::string to_string(entity_ast const*) const;
        std::strong_ordering operator<=>(class_entity_ast const&) const = default;
    };

} // namespace quxlang

#endif // QUXLANG_CLASS_ENTITY_AST_HEADER_GUARD
