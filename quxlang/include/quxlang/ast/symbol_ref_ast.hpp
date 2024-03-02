//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_SYMBOL_REF_AST_HEADER_GUARD
#define QUXLANG_SYMBOL_REF_AST_HEADER_GUARD

#include <string>

namespace quxlang
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
} // namespace quxlang
#endif // QUXLANG_SYMBOL_REF_AST_HEADER_GUARD
