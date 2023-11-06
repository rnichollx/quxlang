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
        std::size_t offset;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_FRAME_VARIABLE_HEADER
