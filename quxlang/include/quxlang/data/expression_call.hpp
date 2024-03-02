//
// Created by Ryan Nicholl on 10/25/23.
//

#ifndef QUXLANG_EXPRESSION_CALL_HEADER_GUARD
#define QUXLANG_EXPRESSION_CALL_HEADER_GUARD

#include "quxlang/data/expression.hpp"
#include <vector>

namespace quxlang
{
    struct expression_call
    {
        expression callee;
        std::vector< expression > args;

        std::strong_ordering operator<=>(const expression_call& other) const = default;
    };
} // namespace quxlang

#endif // QUXLANG_EXPRESSION_CALL_HEADER_GUARD
