//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef QUXLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD
#define QUXLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD

#include "expression.hpp"

namespace quxlang
{
    struct expression_numeric_literal
    {
        std::string value;
        std::optional< std::string > type;
    };
}

#endif // QUXLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD
