//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER
#define RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER

#include "type_ref_ast.hpp"
#include <string>
namespace rylang
{
    struct pointer_ref_ast
    {
        type_ref_ast type;
        inline std::string to_string() const
        {
            return "ast_pointer_ref{ type: " + type.to_string() + " }";
        }
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER
