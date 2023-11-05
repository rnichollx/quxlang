//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER
#define RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER

#include "type_ref_ast.hpp"
#include <string>
#include <compare>
namespace rylang
{
    struct pointer_ref_ast
    {
        qualified_symbol_reference type;

        bool operator < (pointer_ref_ast const& other) const
        {
            return type < other.type;
        }
        //auto operator<=>(const pointer_ref_ast&) const = default;
    };

} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_POINTER_REF_AST_HEADER
