//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER
#define RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER



#include "compiler_fwd.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/cpu_architecture.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/vm_procedure.hpp"
#include "rylang/llvmg/vm_llvm_frame.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

namespace llvm
{
    class PointerType;
    class IntegerType;
}
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
        llvm::IntegerType* get_llvm_int_type_ptr(llvm::LLVMContext& context, primitive_type_integer_reference t);

        llvm::PointerType * get_llvm_type_opaque_ptr(llvm::LLVMContext &context);
        llvm::Type * get_llvm_intptr(llvm::LLVMContext &context);


        llvm::Type * get_llvm_type_from_vm_type(llvm::LLVMContext &context, qualified_symbol_reference typ);
        llvm::Type * get_llvm_type_from_vm_storage(llvm::LLVMContext &context, vm_allocate_storage typ);
        llvm::FunctionType * get_llvm_type_from_func_symbol(llvm::LLVMContext &context, qualified_symbol_reference typ);
        llvm::FunctionType * get_llvm_type_from_func_interface(llvm::LLVMContext &context, vm_procedure_interface ifc);


      public:
        std::vector< std::byte > get_function_code(cpu_arch cpu_type, vm_procedure vmf);
        bool generate_code(llvm::LLVMContext & context, llvm::BasicBlock*& p_block, rylang::vm_block const& block, rylang::vm_llvm_frame& frame);
        void generate_arg_push(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, rylang::vm_procedure procedure, rylang::vm_llvm_frame& frame);
        void generate_ret_storage(llvm::LLVMContext & context, llvm::BasicBlock* p_block, llvm::Function* p_function, rylang::vm_procedure procedure, rylang::vm_llvm_frame& frame);
        llvm::Value* get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, rylang::vm_llvm_frame& frame, rylang::vm_value const& value);
    };
} // namespace rylang

#endif // RPNX_RYANSCRIPT1031_LLVM_CODE_GENERATOR_HEADER
