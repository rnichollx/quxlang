//
// Created by Ryan Nicholl on 12/10/23.
//

#ifndef QUXLANG_AST2_AST2_TYPE_MAP_HEADER_GUARD
#define QUXLANG_AST2_AST2_TYPE_MAP_HEADER_GUARD

#include <compare>
#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    struct ast2_type_map
    {
        std::set<std::string> class_keywords;
        std::vector<std::string> field_decl_order;
        std::map<std::string, ast2_map_entity> members;
        std::map<std::string, ast2_map_entity> globals;

        std::strong_ordering operator<=>(const ast2_type_map& other) const = default;
    };
} // namespace quxlang

#endif // AST2_CLASS_HEADER_GUARD
