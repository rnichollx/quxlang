//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER
#define RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER

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

#endif // RPNX_RYANSCRIPT1031_CLASS_MEMBER_VARIABLE_DECLARATION_HEADER
