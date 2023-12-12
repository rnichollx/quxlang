//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RYLANG_VM_LLVM_FRAME_ITEM_HEADER_GUARD
#define RYLANG_VM_LLVM_FRAME_ITEM_HEADER_GUARD

#include <llvm/IR/Value.h>
namespace rylang
{
   struct vm_llvm_frame_item
    {
       llvm::Value * get_address = nullptr;
     //  llvm::Value * get_value = nullptr;
       llvm::Type * type = nullptr;
       llvm::Align align;
   };
}

#endif // RYLANG_VM_LLVM_FRAME_ITEM_HEADER_GUARD
