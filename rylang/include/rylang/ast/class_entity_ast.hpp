//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER

#include <string>
#include <compare>
#include <set>

namespace rylang
{
    struct entity_ast;
    struct class_entity_ast
    {
        std::set< std::string > m_keywords;
        std::string to_string(entity_ast const*) const;
        std::strong_ordering operator<=>(class_entity_ast const&) const = default;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
