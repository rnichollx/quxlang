//
// Created by Ryan Nicholl on 10/26/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_NUMERIC_LITERAL_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_NUMERIC_LITERAL_HEADER

#include "expression.hpp"

namespace rylang
{
    struct expression_numeric_literal
    {
        std::string value;
        std::optional< std::string > type;
    };
}

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_NUMERIC_LITERAL_HEADER
