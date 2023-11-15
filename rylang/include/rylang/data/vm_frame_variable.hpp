//
// Created by Ryan Nicholl on 11/5/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER
#define RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER

#include <string>
#include <optional>

namespace rylang
{
    struct vm_frame_variable
    {
        std::string name;
        qualified_symbol_reference type;
        std::optional<vm_value> get_addr;
        std::optional<std::string> destructor;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER
