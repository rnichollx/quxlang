//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RYLANG_VM_FRAME_VARIABLE_HEADER_GUARD
#define RYLANG_VM_FRAME_VARIABLE_HEADER_GUARD

#include <optional>
#include <string>

namespace rylang
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
} // namespace rylang

#endif // RYLANG_VM_FRAME_VARIABLE_HEADER_GUARD
