//
// Created by Ryan Nicholl on 8/11/23.
//

#ifndef QUXLANG_DATA_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD
#define QUXLANG_DATA_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD

#include "quxlang/data/class_member_variable_declaration.hpp"
#include <vector>

namespace quxlang
{
    struct class_member_variable_list
    {
        std::vector< class_member_variable_declaration > member_variables;
    };
} // namespace quxlang

#endif // QUXLANG_CLASS_MEMBER_VARIABLE_LIST_HEADER_GUARD
