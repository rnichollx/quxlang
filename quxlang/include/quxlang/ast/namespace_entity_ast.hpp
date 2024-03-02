//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef QUXLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD
#define QUXLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD

#include <string>

namespace quxlang

{
    struct namespace_entity_ast
    {
        std::string to_string(entity_ast const *) const
        {
            return "namespace_ast{}";
        }

        bool operator < (namespace_entity_ast const& other) const
        {
            return false;
        }
    };
} // namespace quxlang

#endif // QUXLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD
