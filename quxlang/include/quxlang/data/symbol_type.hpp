//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef QUXLANG_DATA_SYMBOL_TYPE_HEADER_GUARD
#define QUXLANG_DATA_SYMBOL_TYPE_HEADER_GUARD

namespace quxlang
{
    enum class symbol_kind {
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

#endif // QUXLANG_SYMBOL_TYPE_HEADER_GUARD
