//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
#define RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
#include <string>
namespace rylang
{
    struct null_object_ast
    {
        inline std::string to_string() const
        {
            return "ast_null_object{}";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_NULL_OBJECT_AST_HEADER
