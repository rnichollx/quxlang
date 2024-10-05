//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef QUXLANG_DATA_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD
#define QUXLANG_DATA_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD

#include <cstdint>
#include <string>

#include "quxlang/data/symbol_id.hpp"

namespace quxlang
{
    struct class_member_variable_declaration
    {
        std::string name;
        type_ref_ast typeref;
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD
