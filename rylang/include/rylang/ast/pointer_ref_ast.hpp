//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RYLANG_POINTER_REF_AST_HEADER_GUARD
#define RYLANG_POINTER_REF_AST_HEADER_GUARD

#include "type_ref_ast.hpp"
#include <compare>
#include <string>
namespace rylang
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

} // namespace rylang

#endif // RYLANG_POINTER_REF_AST_HEADER_GUARD
