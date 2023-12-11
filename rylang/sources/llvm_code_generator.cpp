//
// Created by Ryan Nicholl on 10/28/23.
//
#include "rylang/llvm_code_generator.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/llvm_proxy_types.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"
#include "rylang/llvmg/vm_llvm_frame.hpp"
#include "rylang/manipulators/qmanip.hpp"
#include "rylang/manipulators/vm_type_alignment.hpp"
#include "rylang/manipulators/vmmanip.hpp"
#include "rylang/to_pretty_string.hpp"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/PromoteMemToReg.h"
#include <iostream>

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

#include "llvm/Transforms/InstCombine/InstCombine.h"

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SmallVectorMemoryBuffer.h>

std::vector< std::byte > rylang::llvm_code_generator::get_function_code(cpu_arch cpu_type, rylang::vm_procedure vmf)
{
    // TODO: support multiple CPU architectures

    std::unique_ptr< llvm::LLVMContext > context_ptr = std::make_unique< llvm::LLVMContext >();
    vm_llvm_frame frame;
    llvm::LLVMContext& context = *context_ptr;
    // llvm::IRBuilder<> builder(context);

    frame.module = std::make_unique< llvm::Module >("MODULE" + vmf.mangled_name, context);

    // TODO: This is placeholder
    frame.module->setDataLayout("e-m:o-i64:64-i128:128-n32:64-S128");

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

    std::cout << "New function name: " << vmf.mangled_name << std::endl;
    llvm::Function* func = llvm::Function::Create(llvm::FunctionType::get(func_llvm_return_type, func_llvm_arg_types, false), llvm::Function::ExternalLinkage, vmf.mangled_name, frame.module.get());

    llvm::BasicBlock* storage = llvm::BasicBlock::Create(context, "storage", func);
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);

    frame.storage_block = storage;

    llvm::BasicBlock* p_block = entry;

    std::string function_code_stirng = to_pretty_string(vmf.body);
    // std::cout << function_code_stirng << std::endl;
    generate_arg_push(context, storage, func, vmf, frame);
    generate_code(context, p_block, vmf.body, frame, vmf);

    llvm::IRBuilder<> builder(storage);
    builder.CreateBr(entry);

    func->print(llvm::outs());

    llvm::legacy::FunctionPassManager FPM(func->getParent());

    // Add necessary analysis passes that mem2reg might require
    FPM.add(llvm::createBasicAAWrapperPass());
    FPM.add(llvm::createVerifierPass());
    FPM.add(llvm::createInstructionCombiningPass());

    // Promote allocas to registers.
    FPM.add(llvm::createPromoteMemoryToRegisterPass());

    FPM.add(llvm::createSROAPass());
    // FPM.add(new llvm::SimplifyCFGPass());
    /*
        // Do the transformations
        FPM.doInitialization();
        FPM.run(*func);
        FPM.run(*func);
        FPM.doFinalization();
        */

    // Create the analysis managers.
    // These must be declared in this order so that they are destroyed in the
    // correct order due to inter-analysis-manager references.
    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    // Create the new pass manager builder.
    // Take a look at the PassBuilder constructor parameters for more
    // customization, e.g. specifying a TargetMachine or various debugging
    // options.
    llvm::PassBuilder PB;

    // Register all the basic analyses with the managers.
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    // Create the pass manager.
    // This one corresponds to a typical -O2 optimization pipeline.
    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

    // Optimize the IR!
    MPM.run(*frame.module, MAM);

    func->print(llvm::outs());

    // Serialize the module to a SmallVector
    llvm::SmallVector< char, 128 > buffer;
    llvm::raw_svector_ostream OS(buffer);
    llvm::WriteBitcodeToFile(*frame.module, OS);

    // Convert the SmallVector to std::vector<std::byte>
    std::vector< std::byte > bytecodeVector;
    bytecodeVector.reserve(buffer.size());
    for (char c : buffer)
    {
        bytecodeVector.push_back(static_cast< std::byte >(c));
    }

    frame.module = nullptr;
    // llvm::orc::LLJITBuilder jitbuilder;
    // auto jite = jitbuilder.create();
    // if (auto err = jite.takeError())
    //{
    //     std::cerr << "Failed to create JIT" << std::endl;
    //     return {};
    // }

    return bytecodeVector;

    // std::unique_ptr< llvm::orc::LLJIT > jit = std::move(jite.get());

    // Add the module to the JIT
    // if (jit->addIRModule(llvm::orc::ThreadSafeModule(std::move(frame.module), std::move(context_ptr))))
    //{
    //    std::cerr << "Failed to add module to JIT" << std::endl;
    //    return {};
    //}

    // auto symbolE = jit->lookup("main");
    // if (!symbolE)
    //{
    //     std::cerr << "Function not found" << std::endl;
    //     return {};
    // }
    // auto symbol = symbolE.get();

    // auto my_function = symbol.toPtr< std::int32_t (*)(std::int32_t, std::int32_t) >();
    // int resultValue = my_function(4, 5);

    // std::cout << "my_function(4, 5) = " << resultValue << std::endl;

    // return {};
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

bool rylang::llvm_code_generator::generate_code(llvm::LLVMContext& context, llvm::BasicBlock*& p_block, rylang::vm_block const& block, rylang::vm_llvm_frame& frame, rylang::vm_procedure& vmf)
{

    for (vm_executable_unit const& ex : block.code)
    {
        if (typeis< vm_block >(ex))
        {
            // TODO: consider adding separate frame info here
            vm_block const& block2 = boost::get< vm_block >(ex);
            if (!generate_code(context, p_block, block2, frame, vmf))
            {
                return false;
            }
        }
        else if (ex.type() == boost::typeindex::type_id< vm_allocate_storage >())
        {
            assert(false);
        }
        else if (ex.type() == boost::typeindex::type_id< vm_store >())
        {
            llvm::IRBuilder<> builder(p_block);
            vm_store const& store = boost::get< vm_store >(ex);

            llvm::Value* where = get_llvm_value(context, builder, frame, store.where);
            llvm::Value* what = get_llvm_value(context, builder, frame, store.what);

            llvm::Align align = llvm::Align(vm_type_alignment(store.type));
            builder.CreateAlignedStore(what, where, align);
        }
        else if (ex.type() == boost::typeindex::type_id< vm_return >())
        {
            // std::cout << " handle vm return " << std::endl;
            rylang::vm_return ret = boost::get< rylang::vm_return >(ex);

            if (vmf.interface.return_type.has_value())
            {
                // TODO: support void return
                llvm::IRBuilder<> builder(p_block);

                llvm::Align ret_align = frame.values.at(0).align;
                llvm::Value* ret_val_reference = frame.values.at(0).get_address;
                llvm::Type* ret_type = frame.values.at(0).type;
                llvm::Value* ret_val = builder.CreateAlignedLoad(ret_type, ret_val_reference, ret_align);
                // get a true value

                builder.CreateRet(ret_val);
            }
            else
            {
                llvm::IRBuilder<> builder(p_block);
                builder.CreateRetVoid();
            }
            return false;
        }
        else if (typeis< vm_execute_expression >(ex))
        {
            vm_execute_expression const& expr = boost::get< vm_execute_expression >(ex);
            llvm::IRBuilder<> builder(p_block);
            get_llvm_value(context, builder, frame, expr.expr);
        }
        else if (typeis< vm_if >(ex))
        {
            // std::cout << "generate if " << p_block << std::endl;
            vm_if const& if_ = boost::get< vm_if >(ex);
            llvm::IRBuilder<> builder(p_block);

            llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(context, "cond", p_block->getParent());
            builder.CreateBr(cond_block);
            builder.SetInsertPoint(cond_block);

            llvm::Value* cond = get_llvm_value(context, builder, frame, if_.condition);

            llvm::BasicBlock* then_block = llvm::BasicBlock::Create(context, "then", p_block->getParent());
            llvm::BasicBlock* else_block = llvm::BasicBlock::Create(context, "else", p_block->getParent());

            builder.CreateCondBr(cond, then_block, else_block);

            llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "after", p_block->getParent());

            bool if_has_terminator = false;
            bool else_has_terminator = false;

            vm_block const& block2 = if_.then_block;
            if_has_terminator = !generate_code(context, then_block, block2, frame, vmf);

            if (if_.else_block.has_value())
            {
                vm_block const& block3 = if_.else_block.value();
                else_has_terminator = !generate_code(context, else_block, block3, frame, vmf);
            }

            // jump to after block

            if (!if_has_terminator)
            {
                builder.SetInsertPoint(then_block);
                builder.CreateBr(after_block);
            }
            if (!else_has_terminator)
            {
                builder.SetInsertPoint(else_block);
                builder.CreateBr(after_block);
            }
            p_block = after_block;

            // std::cout << "generate if, after: " << p_block << std::endl;
        }
        else if (typeis< vm_while >(ex))
        {
            vm_while const& while_ = boost::get< vm_while >(ex);
            llvm::IRBuilder<> builder(p_block);

            llvm::BasicBlock* cond_block = llvm::BasicBlock::Create(context, "cond", p_block->getParent());
            builder.CreateBr(cond_block);
            builder.SetInsertPoint(cond_block);

            llvm::Value* cond = get_llvm_value(context, builder, frame, while_.condition);

            llvm::BasicBlock* loop_block = llvm::BasicBlock::Create(context, "loop", p_block->getParent());

            llvm::BasicBlock* after_block = llvm::BasicBlock::Create(context, "after", p_block->getParent());

            builder.CreateCondBr(cond, loop_block, after_block);

            bool while_body_has_terminator = false;

            vm_block const& block4 = while_.loop_block;
            while_body_has_terminator = !generate_code(context, loop_block, block4, frame, vmf);
            // jump to after block

            if (!while_body_has_terminator)
            {
                builder.SetInsertPoint(loop_block);
                builder.CreateBr(cond_block);
            }

            p_block = after_block;
        }
        else
        {
            rpnx::unimplemented();
        }
    }

    return true;
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

    if (procedure.interface.return_type.has_value() && !typeis< void_type >(procedure.interface.return_type.value()))
    {
        auto arg_type = procedure.interface.return_type.value();
        llvm::Type* arg_llvm_type = get_llvm_type_from_vm_type(context, arg_type);
        llvm::Value* one_value = llvm::ConstantInt::get(context, llvm::APInt(64, 1));

        llvm::Align arg_align = llvm::Align(vm_type_alignment(arg_type));
        llvm::AllocaInst* alloca = builder.CreateAlloca(arg_llvm_type, 0, one_value);
        vm_llvm_frame_item item;
        item.type = arg_llvm_type;
        item.get_address = alloca;
        item.align = arg_align;
        frame.values.push_back(item);
    }

    for (auto arg_type : procedure.interface.argument_types)
    {
        assert(arg_it != arg_end);
        llvm::Type* arg_llvm_type = get_llvm_type_from_vm_type(context, arg_type);
        llvm::Value* arg_value = &*arg_it;

        llvm::Align arg_align = llvm::Align(vm_type_alignment(arg_type));
        llvm::Value* one_value = llvm::ConstantInt::get(context, llvm::APInt(64, 1));

        llvm::AllocaInst* alloca = builder.CreateAlloca(arg_llvm_type, 0, one_value);
        vm_llvm_frame_item item;
        item.type = arg_llvm_type;
        item.get_address = alloca;
        item.align = arg_align;
        frame.values.push_back(item);
        // store the value in the alloca
        builder.CreateAlignedStore(arg_value, alloca, arg_align);
        arg_it++;
    }

    for (vm_allocate_storage ex : procedure.storage)
    {
        if (ex.kind == storage_type::return_value || ex.kind == storage_type::argument)
        {
            continue;
        }
        vm_allocate_storage const& alloc = ex;

        // The llvm type doesn't contain alignment information, so we have to do it manually in the alloca instruction.
        std::size_t align = alloc.align;

        llvm::Type* StorageType = get_llvm_type_from_vm_storage(context, alloc);

        llvm::Value* one = llvm::ConstantInt::get(context, llvm::APInt(32, 1));

        // Allocate stack space with allocainst
        llvm::IRBuilder<> builder(frame.storage_block);
        llvm::AllocaInst* alloca = builder.Insert(new llvm::AllocaInst(StorageType, 0, one, llvm::Align(align)));
        vm_llvm_frame_item item;
        item.type = StorageType;
        item.get_address = alloca;
        item.align = llvm::Align(align);
        frame.values.push_back(item);
    }
}
llvm::Value* rylang::llvm_code_generator::get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, rylang::vm_llvm_frame& frame, rylang::vm_value const& value)
{

    // std::cout << "Gen llvm value for: " << rylang::to_string(value) << std::endl;

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

        std::string lhs_type_str = to_string(lhs_vm_type);
        std::string rhs_type_str = to_string(rhs_vm_type);

        std::string expr_string = rylang::to_string(binop);

        if (binop.oper == "+")
        {
            result = builder.CreateAdd(lhs, rhs);
            return result;
        }
        else if (binop.oper == "-")
        {
            result = builder.CreateSub(lhs, rhs);
            return result;
        }
        else if (binop.oper == "*")
        {
            result = builder.CreateMul(lhs, rhs);
            return result;
        }
        else if (binop.oper == "==")
        {
            result = builder.CreateICmpEQ(lhs, rhs);
            return result;
        }
        else if (binop.oper == ":=")
        {
            auto underlying_type = remove_ref(lhs_vm_type);
            result = builder.CreateAlignedStore(rhs, lhs, llvm::Align(vm_type_alignment(underlying_type)));
            return result;
        }
        else if (binop.oper == ">")
        {
            result = builder.CreateICmpSGT(lhs, rhs);
            return result;
        }
        else if (binop.oper == "<")
        {
            result = builder.CreateICmpSLT(lhs, rhs);
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
    else if (typeis< vm_expr_call >(value))
    {
        rylang::vm_expr_call const& call = boost::get< rylang::vm_expr_call >(value);

        // TODO: Implement this

        std::string call_expr_str = to_string(call);

        std::vector< llvm::Value* > args;
        for (auto arg : call.arguments)
        {
            args.push_back(get_llvm_value(context, builder, frame, arg));
        }

        std::cout << "mangled name: " << call.mangled_procedure_name << std::endl;

        llvm::FunctionType* funcType = get_llvm_type_from_func_interface(context, call.interface);
        llvm::Function* externalFunction = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, call.mangled_procedure_name, frame.module.get());

        // TODO: Add stack unwinding support
        return builder.CreateCall(externalFunction, args);
    }
    else if (typeis< vm_expr_load_literal >(value))
    {
        vm_expr_load_literal lit = boost::get< vm_expr_load_literal >(value);

        if (lit.type.type() == boost::typeindex::type_id< primitive_type_integer_reference >())
        {
            primitive_type_integer_reference int_type = boost::get< primitive_type_integer_reference >(lit.type);
            llvm::Type* llvm_int_type = get_llvm_int_type_ptr(context, int_type);
            std::string value_string = lit.literal;
            std::uint64_t value_number = std::stoull(value_string);
            return llvm::ConstantInt::get(llvm_int_type, value_number, int_type.has_sign);
        }
        if (lit.type.type() == boost::typeindex::type_id< instance_pointer_type >())
        {
            if (lit.literal == "NULLPTR")
            {
                llvm::Type* llvm_int_type = get_llvm_intptr(context);
                auto nullval = llvm::ConstantInt::get(llvm_int_type, 0);

                llvm::Value* nullpt = builder.CreateIntToPtr(nullval, get_llvm_type_opaque_ptr(context));
                return nullpt;
            }
            else
            {
                assert(false);
            }
        }

        else
        {
            assert(false);
        }
    }
    else if (typeis< vm_expr_access_field >(value))
    {
        vm_expr_access_field const& field = boost::get< vm_expr_access_field >(value);

        llvm::Value* offset = llvm::ConstantInt::get(get_llvm_intptr(context), field.offset);

        llvm::Value* original_ptr = get_llvm_value(context, builder, frame, field.base);

        llvm::Value* new_ptr = builder.CreateGEP(get_llvm_type_from_vm_type(context, field.type), original_ptr, offset);

        return new_ptr;
    }
    else if (typeis< void_value >(value))
    {
        return nullptr;
    }
    else if (typeis< vm_expr_reinterpret >(value))
    {
        vm_expr_reinterpret const& reinterp = boost::get< vm_expr_reinterpret >(value);

        // TODO: Consider checking that the llvm types are the same

        return get_llvm_value(context, builder, frame, reinterp.expr);
    }
    else if (typeis< vm_expr_poison >(value))
    {
        auto type = vm_value_type(value);
        llvm::Type* llvm_type = get_llvm_type_from_vm_type(context, type);
        return llvm::PoisonValue::get(llvm_type);
    }

    assert(false);
    return nullptr;
}

llvm::FunctionType* rylang::llvm_code_generator::get_llvm_type_from_func_symbol(llvm::LLVMContext& context, rylang::qualified_symbol_reference typ)
{
    return nullptr;
}

llvm::FunctionType* rylang::llvm_code_generator::get_llvm_type_from_func_interface(llvm::LLVMContext& context, rylang::vm_procedure_interface ifc)
{
    std::vector< llvm::Type* > arg_types;
    llvm::Type* return_type{};
    for (auto arg_type : ifc.argument_types)
    {
        arg_types.push_back(get_llvm_type_from_vm_type(context, arg_type));
    }
    if (ifc.return_type.has_value())
    {
        return_type = get_llvm_type_from_vm_type(context, ifc.return_type.value());
    }
    else
    {
        return_type = llvm::Type::getVoidTy(context);
    }

    return llvm::FunctionType::get(return_type, arg_types, false);
}

llvm::Type* rylang::llvm_code_generator::get_llvm_intptr(llvm::LLVMContext& context)
{
    // TODO: check the actual bitwidth on the current platform
    return llvm::IntegerType::get(context, 64);
}
