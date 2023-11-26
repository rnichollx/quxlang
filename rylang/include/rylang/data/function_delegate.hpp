//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef RYLANG_FUNCTION_DELEGATE_HPP
#define RYLANG_FUNCTION_DELEGATE_HPP

#include "expression.hpp"
#include "qualified_symbol_reference.hpp"

namespace rylang
{
    struct function_delegate
    {
        qualified_symbol_reference target;
        std::vector< expression > args;
    };
} // namespace rylang

#endif // FUNCTION_DELEGATE_HPP
