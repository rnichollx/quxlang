//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef QUXLANG_VM_FRAME_VARIABLE_HEADER_GUARD
#define QUXLANG_VM_FRAME_VARIABLE_HEADER_GUARD

#include <optional>
#include <string>

namespace quxlang
{
    struct vm_frame_variable
    {
        std::string name;
        type_symbol type;
        bool is_temporary = false;
        vm_value get_addr;

        vm_allocate_storage storage;
    };

    struct vm_frame_variable_state
    {
        bool alive = false;
        bool this_frame = true;
    };
} // namespace quxlang

#endif // QUXLANG_VM_FRAME_VARIABLE_HEADER_GUARD
