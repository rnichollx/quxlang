//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef RYLANG_FUNCTION_DELEGATE_HEADER_GUARD
#define RYLANG_FUNCTION_DELEGATE_HEADER_GUARD

#include "expression.hpp"
#include "qualified_symbol_reference.hpp"

namespace rylang
{
    struct function_delegate
    {
        type_symbol target;
        std::vector< expression > args;

        std::strong_ordering operator<=>(const function_delegate& other) const = default;
    };
} // namespace rylang

#endif // FUNCTION_DELEGATE_HEADER_GUARD
