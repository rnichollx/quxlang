// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BACKENDS_LLVM_VM_LLVM_FRAME_ITEM_HEADER_GUARD
#define QUXLANG_BACKENDS_LLVM_VM_LLVM_FRAME_ITEM_HEADER_GUARD

#include <llvm/IR/Value.h>
namespace quxlang
{
   struct vm_llvm_frame_item
    {
       llvm::Value * get_address = {};
       llvm::Type * type = {};
       llvm::Align align = {};
   };
}

#endif // QUXLANG_VM_LLVM_FRAME_ITEM_HEADER_GUARD
