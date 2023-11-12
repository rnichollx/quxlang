//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_EXPRESSION_DOTREFERENCE_HEADER
#define RPNX_RYANSCRIPT1031_EXPRESSION_DOTREFERENCE_HEADER

#include "expression.hpp"

namespace rylang
{
    struct expression_dotreference
    {
        expression lhs;
        std::string field_name;

        std::strong_ordering operator<=>(const expression_dotreference& other) const = default;
    };
}

#endif // RPNX_RYANSCRIPT1031_EXPRESSION_DOTREFERENCE_HEADER
