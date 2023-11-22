//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER
#define RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER

#include <optional>
#include <string>

namespace rylang
{
    struct vm_frame_variable
    {
        std::string name;
        qualified_symbol_reference type;
        bool is_temporary = false;
        vm_value get_addr;
    };

    struct vm_frame_variable_state
    {
        bool alive = false;
        bool this_frame = true;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER
