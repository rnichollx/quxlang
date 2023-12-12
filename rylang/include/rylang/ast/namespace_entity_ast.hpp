//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RYLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD
#define RYLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD

#include <string>

namespace rylang

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
} // namespace rylang

#endif // RYLANG_NAMESPACE_ENTITY_AST_HEADER_GUARD
