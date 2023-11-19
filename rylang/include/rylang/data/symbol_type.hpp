//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef RPNX_RYANSCRIPT1031_SYMBOL_TYPE_HEADER
#define RPNX_RYANSCRIPT1031_SYMBOL_TYPE_HEADER

namespace rylang
{
    enum class symbol_type {
        class_type,
        functum_type,
        function_type,
        funtanoid_type,
        variable_type,
        member_variable_type,
        member_functum_type,
        member_function_type,
        member_functanoid_type,
        pseudo_type,
        primitive_type,
        invalid_symbol_reference,
    };
}

#endif // RPNX_RYANSCRIPT1031_SYMBOL_TYPE_HEADER
