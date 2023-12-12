//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef RYLANG_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD
#define RYLANG_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD

#include "rylang/data/class_member_variable_declaration.hpp"
#include <vector>

namespace rylang
{
    struct class_member_variable_list
    {
        std::vector< class_member_variable_declaration > member_variables;
    };
} // namespace rylang

#endif // RYLANG_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD
