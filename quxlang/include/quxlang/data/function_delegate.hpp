//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef QUXLANG_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_FUNCTION_DELEGATE_HEADER_GUARD

#include "expression.hpp"
#include "type_symbol.hpp"

namespace quxlang
{
    struct function_delegate
    {
        type_symbol target;
        std::vector< expression > args;

        std::strong_ordering operator<=>(const function_delegate& other) const
        {
            return rpnx::compare(target, other.target, args, other.args);
        }
    };
} // namespace quxlang

#endif // FUNCTION_DELEGATE_HEADER_GUARD
