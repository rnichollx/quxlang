//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef RYLANG_LLVM_CODE_GENERATOR_HEADER_GUARD
#define RYLANG_LLVM_CODE_GENERATOR_HEADER_GUARD

#include "rylang/ast2/ast2_entity.hpp"
#include "rylang/backends/llvm/vm_llvm_frame.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/canonical_resolved_function_chain.hpp"
#include "rylang/data/cpu_architecture.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/vm_procedure.hpp"
#include "rylang/manipulators/llvm_lookup.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

namespace llvm
{
    class PointerType;
    class IntegerType;
    class Target;
    class TargetMachine;
} // namespace llvm
namespace rylang
{
    class llvm_code_generator
    {
        output_info m_machine_info;
        std::string target_triple_str = "armv8-a-unknown-unknown-unknown";
        llvm::Target const* target;

        llvm::TargetMachine* target_machine;

      public:
        llvm_code_generator(output_info m);

      private:
        llvm::IntegerType* get_llvm_int_type_ptr(llvm::LLVMContext& context, primitive_type_integer_reference t);

        llvm::PointerType* get_llvm_type_opaque_ptr(llvm::LLVMContext& context);
        llvm::Type* get_llvm_intptr(llvm::LLVMContext& context);

        llvm::Type* get_llvm_type_from_vm_type(llvm::LLVMContext& context, type_symbol typ);
        llvm::Type* get_llvm_type_from_vm_storage(llvm::LLVMContext& context, vm_allocate_storage typ);
        llvm::FunctionType* get_llvm_type_from_func_symbol(llvm::LLVMContext& context, type_symbol typ);
        llvm::FunctionType* get_llvm_type_from_func_interface(llvm::LLVMContext& context, vm_procedure_interface ifc);

      public:
        std::vector< std::byte > get_function_code(cpu_arch cpu_type, vm_procedure vmf);
        std::vector< std::byte > assemble(asm_procedure input, rylang::cpu cpu_type);

        void foo();

        bool generate_code(llvm::LLVMContext& context, llvm::BasicBlock*& p_block, rylang::vm_block const& block, rylang::vm_llvm_frame& frame, vm_procedure& vmf);
        void generate_arg_push(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, rylang::vm_procedure procedure, rylang::vm_llvm_frame& frame);
        void generate_ret_storage(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, rylang::vm_procedure procedure, rylang::vm_llvm_frame& frame);
        llvm::Value* get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, rylang::vm_llvm_frame& frame, rylang::vm_value const& value);
    };
} // namespace rylang

#endif // RYLANG_LLVM_CODE_GENERATOR_HEADER_GUARD
