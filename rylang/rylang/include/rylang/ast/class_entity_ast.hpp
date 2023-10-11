//
// Created by Ryan Nicholl on 7/29/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER

#include <string>

namespace rylang
{
    struct entity_ast;
    struct class_entity_ast
    {
        std::string to_string(entity_ast const*) const;
        bool operator<(class_entity_ast const& other) const
        {
            return false;
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_CLASS_ENTITY_AST_HEADER
