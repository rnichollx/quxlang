// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_BACKENDS_LLVM_LLVM_CODE_GENERATOR_HEADER_GUARD
#define QUXLANG_BACKENDS_LLVM_LLVM_CODE_GENERATOR_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include "quxlang/backends/llvm/vm_llvm_frame.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_resolved_function_chain.hpp"
#include "quxlang/data/cpu_architecture.hpp"
#include "quxlang/data/llvm_proxy_types.hpp"
#include "quxlang/data/vm_procedure.hpp"
#include "quxlang/manipulators/llvm_lookup.hpp"

#include <llvm/IR/IRBuilder.h>
#include <llvm/Support/TargetSelect.h>

namespace llvm
{
    class PointerType;
    class IntegerType;
    class Target;
    class TargetMachine;
} // namespace llvm
namespace quxlang
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
        std::vector< std::byte > qxbc_to_llvm_bc(quxlang::vm_procedure vmf);
        std::vector< std::byte > compile_llvm_ir_to_elf(std::vector<std::byte> ir);
        std::vector< std::byte > assemble(quxlang::asm_procedure input);

        void foo();
      private:

        bool generate_code(llvm::LLVMContext& context, llvm::BasicBlock*& p_block, quxlang::vm_block const& block, quxlang::vm_llvm_frame& frame, vm_procedure& vmf);
        void generate_arg_push(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, quxlang::vm_procedure procedure, quxlang::vm_llvm_frame& frame);
        void generate_ret_storage(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, quxlang::vm_procedure procedure, quxlang::vm_llvm_frame& frame);
        llvm::Value* get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, quxlang::vm_llvm_frame& frame, quxlang::vm_value const& value);
    };
} // namespace quxlang

#endif // QUXLANG_LLVM_CODE_GENERATOR_HEADER_GUARD
