//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_SYMBOL_REF_AST_HEADER_GUARD
#define RYLANG_SYMBOL_REF_AST_HEADER_GUARD

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
        bool operator < (symbol_ref_ast const& other) const
        {
            return name < other.name;
        }
    };
} // namespace rylang
#endif // RYLANG_SYMBOL_REF_AST_HEADER_GUARD
