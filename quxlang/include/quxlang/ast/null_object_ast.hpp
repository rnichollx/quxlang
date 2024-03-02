//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_NULL_OBJECT_AST_HEADER_GUARD
#define QUXLANG_NULL_OBJECT_AST_HEADER_GUARD
#include <string>

#include "quxlang/fwd.hpp"
namespace quxlang
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

        inline bool operator < (null_object_ast const& other) const
        {
            return false;
        }
    };
} // namespace quxlang

#endif // QUXLANG_NULL_OBJECT_AST_HEADER_GUARD
