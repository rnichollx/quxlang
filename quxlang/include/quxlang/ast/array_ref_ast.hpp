//
// Created by Ryan Nicholl on 7/20/23.
//

#ifndef QUXLANG_ARRAY_REF_AST_HEADER_GUARD
#define QUXLANG_ARRAY_REF_AST_HEADER_GUARD

#include "symbol_ref_ast.hpp"
#include <string>
#include <utility>
#include <tuple>
namespace quxlang
{
    struct array_ref_ast
    {
        symbol_ref_ast type;
        std::size_t size;

        inline std::string to_string() const
        {
            return "ast_array_ref{ type: " + type.to_string() + ", size: " + std::to_string(size) + " }";
        }
        bool operator<(array_ref_ast const& other) const
        {
            return std::tie(type, size) < std::tie(other.type, other.size);
        }
    };
} // namespace quxlang

#endif // QUXLANG_ARRAY_REF_AST_HEADER_GUARD
