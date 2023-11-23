//
// Created by Ryan Nicholl on 10/30/23.
//

#ifndef RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_ITEM_HEADER
#define RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_ITEM_HEADER

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

#endif // RPNX_RYANSCRIPT1031_VM_LLVM_FRAME_ITEM_HEADER
