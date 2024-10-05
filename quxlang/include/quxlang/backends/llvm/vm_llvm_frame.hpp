//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef QUXLANG_BACKENDS_LLVM_VM_LLVM_FRAME_HEADER_GUARD
#define QUXLANG_BACKENDS_LLVM_VM_LLVM_FRAME_HEADER_GUARD

#include "vm_llvm_frame_item.hpp"
namespace quxlang
{
    struct vm_llvm_frame
    {
        std::vector< vm_llvm_frame_item > values;

        llvm::BasicBlock * storage_block = nullptr;
        std::unique_ptr<llvm::Module> module;
    };
} // namespace quxlang

#endif // QUXLANG_VM_LLVM_FRAME_HEADER_GUARD
