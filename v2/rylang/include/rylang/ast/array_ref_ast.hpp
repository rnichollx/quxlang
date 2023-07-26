//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_ARRAY_REF_AST_HEADER
#define RPNX_RYANSCRIPT1031_ARRAY_REF_AST_HEADER

#include <string>
#include "symbol_ref_ast.hpp"
namespace rylang
{
    struct array_ref_ast
    {
        symbol_ref_ast type;
        std::size_t size;

        inline std::string to_string() const
        {
            return "ast_array_ref{ type: " + type.to_string() + ", size: " + std::to_string(size) + " }";
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_ARRAY_REF_AST_HEADER
