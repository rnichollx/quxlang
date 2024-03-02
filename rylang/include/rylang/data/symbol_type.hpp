//
// Created by Ryan Nicholl on 11/16/23.
//

#ifndef RYLANG_SYMBOL_TYPE_HEADER_GUARD
#define RYLANG_SYMBOL_TYPE_HEADER_GUARD

namespace rylang
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

#endif // RYLANG_SYMBOL_TYPE_HEADER_GUARD
