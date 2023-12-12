//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RYLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD
#define RYLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD

#include "expression.hpp"

namespace rylang
{
    struct expression_numeric_literal
    {
        std::string value;
        std::optional< std::string > type;
    };
}

#endif // RYLANG_EXPRESSION_NUMERIC_LITERAL_HEADER_GUARD
