//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_SYMBOL_REF_AST_HEADER
#define RPNX_RYANSCRIPT1031_SYMBOL_REF_AST_HEADER

#include <string>

namespace rylang
{
    struct symbol_ref_ast
    {
        std::string name;
        inline std::string to_string() const
        {
            return "ast_symbol_ref{ name: " + name + " }";
        }
    };
} // namespace rylang
#endif // RPNX_RYANSCRIPT1031_SYMBOL_REF_AST_HEADER
