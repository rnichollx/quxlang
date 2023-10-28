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

#include "compiler_fwd.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/cpu_architecture.hpp"
#include "rylang/data/llvm_proxy_types.hpp"

namespace rylang
{
    class llvm_code_generator
    {
        compiler* c;

      public:
        llvm_code_generator(compiler* c)
            : c(c)
        {
            llvm::InitializeNativeTarget();
            llvm::InitializeNativeTargetAsmPrinter();
        }

      private:
        llvm::IntegerType* get_llvm_int_type_ptr(llvm::LLVMContext& context, llvm_proxy_type_int t);

        llvm::PointerType * get_llvm_type_opaque_ptr(llvm::LLVMContext &context);

        llvm::Type * get_llvm_type_from_proxy(llvm::LLVMContext &context, llvm_proxy_type typ);

      public:
        std::vector< std::byte > get_function_code(cpu_arch cpu_type, canonical_resolved_function_chain);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER
