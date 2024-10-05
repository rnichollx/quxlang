//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef QUXLANG_DATA_INTERP_VALUE_HEADER_GUARD
#define QUXLANG_DATA_INTERP_VALUE_HEADER_GUARD

#include "type_symbol.hpp"
#include <rpnx/variant.hpp>
#include <quxlang/data/vm_executable_unit.hpp>

namespace quxlang
{

    struct interp_value
    {
        type_symbol type;
        std::vector< std::byte > data;

        RPNX_MEMBER_METADATA(interp_value, type, data);
    };

    struct expr_interp_input
    {
        type_symbol context;
        expression expression;

        RPNX_MEMBER_METADATA(expr_interp_input, context, expression);
    };

    struct interp_input
    {
        type_symbol context;
        vm_execute_expression expression;

        RPNX_MEMBER_METADATA(interp_input, context, expression);
    };

} // namespace quxlang

#endif // RPNX_QUXLANG_INTERP_VALUE_HEADER
