//
// Created by Ryan Nicholl on 10/28/23.
//
#include "rylang/llvm_code_generator.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/qualified_reference.hpp"
#include "rylang/llvmg/vm_llvm_frame.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vm_type_alignment.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <iostream>

std::vector< std::byte > rylang::llvm_code_generator::get_function_code(cpu_arch cpu_type, rylang::vm_procedure vmf)
{
    // TODO: support multiple CPU architectures
    llvm::LLVMContext context;
    // llvm::IRBuilder<> builder(context);

    std::unique_ptr< llvm::Module > module = std::make_unique< llvm::Module >("main", context);

    // TODO: This is placeholder
    module->setDataLayout("e-m:e-i64:64-i128:128-n32:64-S128");

    std::optional< qualified_symbol_reference > func_return_type = vmf.interface.return_type;

    llvm::Type* func_llvm_return_type = nullptr;

    if (func_return_type.has_value())
    {
        func_llvm_return_type = get_llvm_type_from_vm_type(context, func_return_type.value());
    }
    else
    {
        func_llvm_return_type = llvm::Type::getVoidTy(context);
    }

    std::vector< llvm::Type* > func_llvm_arg_types;

    for (auto arg_type : vmf.interface.argument_types)
    {
        func_llvm_arg_types.push_back(get_llvm_type_from_vm_type(context, arg_type));
    }

    llvm::Function* func = llvm::Function::Create(llvm::FunctionType::get(func_llvm_return_type, func_llvm_arg_types, false), llvm::Function::ExternalLinkage, "main", module.get());

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);
    vm_llvm_frame frame;

    generate_arg_push(context, entry, func, vmf, frame);
    generate_code(context, entry, vmf.body, frame);

    func->print(llvm::outs());

    llvm::legacy::FunctionPassManager FPM(func->getParent());

    // Add necessary analysis passes that mem2reg might require
    FPM.add(llvm::createBasicAAWrapperPass());
    FPM.add(llvm::createVerifierPass());

    // Promote allocas to registers.
    FPM.add(llvm::createPromoteMemoryToRegisterPass());

    FPM.add(llvm::createSROAPass());

    // Do the transformations
    FPM.doInitialization();
    FPM.run(*func);
    FPM.run(*func);
    FPM.doFinalization();

    func->print(llvm::outs());

    // TODO: convert the code to machine/linker code/object and return it

    return {};
}

llvm::Type* rylang::llvm_code_generator::get_llvm_type_from_vm_type(llvm::LLVMContext& ctx, rylang::qualified_symbol_reference typ)
{
    if (typ.type() == boost::typeindex::type_id< rylang::primitive_type_integer_reference >())
    {
        return get_llvm_int_type_ptr(ctx, boost::get< rylang::primitive_type_integer_reference >(typ));
    }
    else if (rylang::is_ref(typ))
    {
        return get_llvm_type_opaque_ptr(ctx);
    }
    else
    {
        throw std::runtime_error("unimplemented");
    }
}

llvm::IntegerType* rylang::llvm_code_generator::get_llvm_int_type_ptr(llvm::LLVMContext& context, rylang::primitive_type_integer_reference t)
{
    assert(t.bits != 0);
    return llvm::IntegerType::get(context, t.bits);
}
llvm::PointerType* rylang::llvm_code_generator::get_llvm_type_opaque_ptr(llvm::LLVMContext& context)
{
    return llvm::PointerType::get(context, 0);
}

void rylang::llvm_code_generator::generate_code(llvm::LLVMContext& context, llvm::BasicBlock* p_block, rylang::vm_block const& block, rylang::vm_llvm_frame& frame)
{
    llvm::IRBuilder<> builder(p_block);
    for (vm_executable_unit const& ex : block.code)
    {
        if (typeis< vm_block >(ex))
        {
            // TODO: consider adding separate frame info here
            vm_block const& block = boost::get< vm_block >(ex);
            generate_code(context, p_block, block, frame);
        }
        else if (ex.type() == boost::typeindex::type_id< vm_allocate_storage >())
        {
            vm_allocate_storage const& alloc = boost::get< vm_allocate_storage >(ex);

            // The llvm type doesn't contain alignment information, so we have to do it manually in the alloca instruction.
            std::size_t align = alloc.align;

            llvm::Type* StorageType = get_llvm_type_from_vm_storage(context, alloc);

            llvm::Value* one = llvm::ConstantInt::get(context, llvm::APInt(32, 1));

            // Allocate stack space with allocainst
            llvm::AllocaInst* alloca = builder.Insert(new llvm::AllocaInst(StorageType, 0, one, llvm::Align(align)));
            vm_llvm_frame_item item;
            item.type = StorageType;
            item.get_address = alloca;
            frame.values.push_back(item);
        }
        else if (ex.type() == boost::typeindex::type_id< vm_store >())
        {
            vm_store const& store = boost::get< vm_store >(ex);

            llvm::Value* where = get_llvm_value(context, builder, frame, store.where);
            llvm::Value* what = get_llvm_value(context, builder, frame, store.what);

            llvm::Align align = llvm::Align(vm_type_alignment(store.type));
            builder.CreateAlignedStore(what, where, align);
        }
        else if (ex.type() == boost::typeindex::type_id< vm_return >())
        {
            rylang::vm_return ret = boost::get< rylang::vm_return >(ex);
            // TODO: support void return
            llvm::Value* ret_val = get_llvm_value(context, builder, frame, ret.expr.value());
            builder.CreateRet(ret_val);
        }
        else if (typeis< vm_execute_expression >(ex))
        {
            vm_execute_expression const& expr = boost::get< vm_execute_expression >(ex);

            get_llvm_value(context, builder, frame, expr.expr);
        }
        else
        {
            // TODO: unimplemented
            assert(false);
        }
    }
}
llvm::Type* rylang::llvm_code_generator::get_llvm_type_from_vm_storage(llvm::LLVMContext& context, vm_allocate_storage typ)
{
    // Just create an array that contains enough bytes

    return llvm::ArrayType::get(llvm::IntegerType::get(context, 8), typ.size);
}
void rylang::llvm_code_generator::generate_arg_push(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, rylang::vm_procedure procedure, rylang::vm_llvm_frame& frame)
{
    // Push the arguments into the frame
    // this doesn't nessecarily mean that the arguments be pushed on the stack, but we need to push
    // their Value* into the frame so that we can access them later.

    llvm::IRBuilder<> builder(p_block);

    auto arg_it = p_function->arg_begin();
    auto arg_end = p_function->arg_end();

    for (auto arg_type : procedure.interface.argument_types)
    {
        assert(arg_it != arg_end);
        llvm::Type* arg_llvm_type = get_llvm_type_from_vm_type(context, arg_type);
        llvm::Value* arg_value = &*arg_it;

        llvm::Align arg_align = llvm::Align(vm_type_alignment(arg_type));
        llvm::AllocaInst* alloca = builder.Insert(new llvm::AllocaInst(arg_llvm_type, 0, arg_value, llvm::Align(arg_align)));
        vm_llvm_frame_item item;
        item.type = arg_llvm_type;
        item.get_address = alloca;
        frame.values.push_back(item);
        // store the value in the alloca
        builder.CreateAlignedStore(arg_value, alloca, arg_align);
        arg_it++;
    }
}
llvm::Value* rylang::llvm_code_generator::get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, rylang::vm_llvm_frame& frame, rylang::vm_value const& value)
{
    if (value.type() == boost::typeindex::type_id< rylang::vm_expr_load_address >())
    {
        rylang::vm_expr_load_address lea = boost::get< rylang::vm_expr_load_address >(value);
        llvm::Value* addr = frame.values.at(lea.index).get_address;
        assert(addr != nullptr);
        return addr;
    }
    else if (value.type() == boost::typeindex::type_id< rylang::vm_expr_dereference >())
    {
        rylang::vm_expr_dereference const& deref = boost::get< rylang::vm_expr_dereference >(value);
        llvm::Type* load_type = get_llvm_type_from_vm_type(context, deref.type);

        llvm::Value* ptr = get_llvm_value(context, builder, frame, deref.expr);
        llvm::Value* load = builder.CreateAlignedLoad(load_type, ptr, llvm::Align(vm_type_alignment(deref.type)));

        return load;
    }
    else if (value.type() == boost::typeindex::type_id< rylang::vm_expr_primitive_binary_op >())
    {
        rylang::vm_expr_primitive_binary_op const& binop = boost::get< rylang::vm_expr_primitive_binary_op >(value);

        llvm::Value* lhs = get_llvm_value(context, builder, frame, binop.lhs);
        llvm::Value* rhs = get_llvm_value(context, builder, frame, binop.rhs);

        llvm::Value* result = nullptr;



        auto lhs_vm_type = vm_value_type(binop.lhs);

        auto rhs_vm_type = vm_value_type(binop.rhs);

        std::string lhs_type_str = boost::apply_visitor(qualified_symbol_stringifier(), lhs_vm_type);
        std::string rhs_type_str = boost::apply_visitor(qualified_symbol_stringifier(), rhs_vm_type);

        std::string expr_string = rylang::to_string(binop);

        if (binop.oper == rylang::vm_primitive_binary_operator::add)
        {
            result = builder.CreateAdd(lhs, rhs);
            return result;
        }

        assert(false);
    }
    else if (typeis< vm_expr_store >(value))
    {
        rylang::vm_expr_store const& store = boost::get< rylang::vm_expr_store >(value);
        llvm::Value* lhs = get_llvm_value(context, builder, frame, store.where);
        llvm::Value* rhs = get_llvm_value(context, builder, frame, store.what);
        llvm::Value* result = nullptr;
        auto underlying_type = remove_ref(store.type);
        result = builder.CreateAlignedStore(rhs, lhs, llvm::Align(vm_type_alignment(underlying_type)));
        return result;
    }

    assert(false);
    return nullptr;
}
