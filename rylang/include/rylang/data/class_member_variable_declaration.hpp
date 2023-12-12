//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RYLANG_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD
#define RYLANG_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD

#include <cstdint>
#include <string>

#include "rylang/data/symbol_id.hpp"

namespace rylang
{
    struct class_member_variable_declaration
    {
        std::string name;
        type_ref_ast typeref;
    };
} // namespace rylang

#endif // RYLANG_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER_GUARD
