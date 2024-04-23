//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_EXPRESSION_DOTREFERENCE_HEADER_GUARD
#define QUXLANG_EXPRESSION_DOTREFERENCE_HEADER_GUARD

#include "expression.hpp"

namespace quxlang
{
    struct expression_dotreference
    {
        expression lhs;
        std::string field_name;

        RPNX_MEMBER_METADATA(expression_dotreference, lhs, field_name);
    };
}

#endif // QUXLANG_EXPRESSION_DOTREFERENCE_HEADER_GUARD
