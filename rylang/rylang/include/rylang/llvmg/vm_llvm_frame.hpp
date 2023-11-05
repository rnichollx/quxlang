//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_HEADER
#define RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_HEADER

#include "vm_llvm_frame_item.hpp"
namespace rylang
{
    struct vm_llvm_frame
    {
        std::vector< vm_llvm_frame_item > values;
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_HEADER
