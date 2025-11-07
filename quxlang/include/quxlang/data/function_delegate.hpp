// Copyright 2023-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_DATA_FUNCTION_DELEGATE_HEADER_GUARD
#define QUXLANG_DATA_FUNCTION_DELEGATE_HEADER_GUARD

#include "expression.hpp"
#include "type_symbol.hpp"
#include <rpnx/compare.hpp>

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
