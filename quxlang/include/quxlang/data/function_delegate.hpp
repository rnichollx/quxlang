//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef QUXLANG_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_FUNCTION_DELEGATE_HEADER_GUARD

#include "expression.hpp"
#include "qualified_symbol_reference.hpp"

namespace quxlang
{
    struct function_delegate
    {
        type_symbol target;
        std::vector< expression > args;

        auto operator<=>(const function_delegate& other) const = default;
    };
} // namespace quxlang

#endif // FUNCTION_DELEGATE_HEADER_GUARD
