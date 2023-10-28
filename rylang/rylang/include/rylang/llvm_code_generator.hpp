//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER
#define RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

namespace rylang
{
    class llvm_code_generator
    {
      public:
        llvm_code_generator()
        {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
        }
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER
