//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_NAMESPACE_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_NAMESPACE_ENTITY_AST_HEADER

#include <string>

namespace rylang

{
    struct namespace_entity_ast
    {
        std::string to_string(entity_ast const *) const
        {
            return "namespace_ast{}";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_NAMESPACE_ENTITY_AST_HEADER
