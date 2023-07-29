//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
#define RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
#include <string>

#include "rylang/fwd.hpp"
namespace rylang
{
    struct null_object_ast
    {
        inline std::string to_string() const
        {
            return "ast_null_object{}";
        }

        inline std::string to_string(entity_ast const *) const
        {
            return "ast_null_object{}";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
