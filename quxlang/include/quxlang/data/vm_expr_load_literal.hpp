// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "type_symbol.hpp"

#ifndef QUXLANG_DATA_VM_EXPR_LOAD_LITERAL_HEADER_GUARD
#define QUXLANG_DATA_VM_EXPR_LOAD_LITERAL_HEADER_GUARD

#include <string>

namespace quxlang
{

    struct vm_expr_literal
    {
        std::string literal;

        RPNX_MEMBER_METADATA(vm_expr_literal, literal);
    };

    struct vm_expr_load_literal
    {
        std::string literal;
        type_symbol type;

        RPNX_MEMBER_METADATA(vm_expr_load_literal, literal, type);
    };


}

#endif // QUXLANG_VM_EXPR_LOAD_LITERAL_HEADER_GUARD