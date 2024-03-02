//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_POINTER_REF_AST_HEADER_GUARD
#define QUXLANG_POINTER_REF_AST_HEADER_GUARD

#include "type_ref_ast.hpp"
#include <compare>
#include <string>
namespace quxlang
{
    struct pointer_ref_ast
    {
        qualified_symbol_reference type;

        bool operator<(pointer_ref_ast const& other) const
        {
            return type < other.type;
        }
        // auto operator<=>(const pointer_ref_ast&) const = default;
    };

} // namespace quxlang

#endif // QUXLANG_POINTER_REF_AST_HEADER_GUARD
