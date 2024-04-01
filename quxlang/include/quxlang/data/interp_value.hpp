//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_INTERP_VALUE_HEADER
#define RPNX_QUXLANG_INTERP_VALUE_HEADER

#include "type_symbol.hpp"
#include <rpnx/variant.hpp>

namespace quxlang
{

    struct interp_value
    {
        type_symbol type;
        std::vector< std::byte > data;

        RPNX_MEMBER_METADATA(interp_value, type, data);
    };

    struct interp_input
    {
        type_symbol context;
        expression expression;

        RPNX_MEMBER_METADATA(interp_input, context, expression);
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_INTERP_VALUE_HEADER
