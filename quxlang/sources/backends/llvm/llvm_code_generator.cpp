// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/backends/llvm/llvm_code_generator.hpp"
#include "quxlang/backends/llvm/vm_llvm_frame.hpp"
#include "quxlang/compiler.hpp"
#include "quxlang/data/code_relocation.hpp"
#include "quxlang/data/llvm_proxy_types.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/manipulators/llvm_symbol_relocation.hpp"
#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/manipulators/vm_type_alignment.hpp"
#include "quxlang/manipulators/vmmanip.hpp"
#include "quxlang/to_pretty_string.hpp"
#include "quxlang/data/output_object_symbol.hpp"
#include "quxlang/manipulators/convert_llvm_object.hpp"
#include "quxlang/backends/asm/arm_asm_converter.hpp"

#pragma warning(push)
#pragma warning(disable:4244)

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCAsmInfo.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCObjectFileInfo.h>
#include <llvm/MC/MCObjectWriter.h>
#include <llvm/MC/MCParser/MCAsmParser.h>
#include <llvm/MC/MCParser/MCTargetAsmParser.h>
#include <llvm/MC/MCRegisterInfo.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/MCTargetOptionsCommandFlags.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/BasicAliasAnalysis.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/MCAsmBackend.h>
#include <llvm/MC/MCCodeEmitter.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/MCInst.h>
#include <llvm/MC/MCInstBuilder.h>
#include <llvm/MC/MCInstrInfo.h>
#include <llvm/MC/MCObjectStreamer.h>
#include <llvm/MC/MCStreamer.h>
#include <llvm/MC/MCSubtargetInfo.h>
#include <llvm/MC/MCTargetOptions.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Scalar/Reassociate.h>
#include <llvm/Transforms/Scalar/SimplifyCFG.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Utils/PromoteMemToReg.h>
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


#include <llvm/Transforms/InstCombine/InstCombine.h>

#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/SmallVectorMemoryBuffer.h>


#include <optional>
#include <fstream>

#pragma warning(pop)

std::vector< std::byte > quxlang::llvm_code_generator::qxbc_to_llvm_bc(quxlang::vm_procedure vmf)
{
    // TODO: support multiple CPU architectures
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::unique_ptr< llvm::LLVMContext > context_ptr = std::make_unique< llvm::LLVMContext >();
    vm_llvm_frame frame;
    llvm::LLVMContext& context = *context_ptr;
    // llvm::IRBuilder<> builder(context);

    frame.module = std::make_unique< llvm::Module >("MODULE" + vmf.mangled_name, context);

    // TODO: This is placeholder
    // frame.module->setDataLayout("e-m:o-i64:64-i128:128-n32:64-S128");

    frame.module->setDataLayout(target_machine->createDataLayout());
    frame.module->setTargetTriple(target_machine->getTargetTriple().str());

    std::optional< type_symbol > func_return_type = vmf.interface.return_type;

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

    for (auto const& [name, type] : vmf.interface.argument_types.named)
    {
        func_llvm_arg_types.push_back(get_llvm_type_from_vm_type(context, type));
    }

    for (auto arg_type : vmf.interface.argument_types.positional)
    {
        func_llvm_arg_types.push_back(get_llvm_type_from_vm_type(context, arg_type));
    }

    QUXLANG_DEBUG({ std::cout << "New function name: " << vmf.mangled_name << std::endl; });
    llvm::Function* func = llvm::Function::Create(llvm::FunctionType::get(func_llvm_return_type, func_llvm_arg_types, false), llvm::Function::ExternalLinkage, vmf.mangled_name, frame.module.get());

    llvm::BasicBlock* storage = llvm::BasicBlock::Create(context, "storage", func);
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);

    frame.storage_block = storage;

    llvm::BasicBlock* p_block = entry;

   // std::string function_code_string = to_pretty_string(vmf.body);
    generate_arg_push(context, storage, func, vmf, frame);
    generate_code(context, p_block, vmf.body, frame, vmf);

    llvm::IRBuilder<> builder(storage);
    builder.CreateBr(entry);

    func->print(llvm::outs());

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

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
    // New:

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

llvm::Type* quxlang::llvm_code_generator::get_llvm_type_from_vm_type(llvm::LLVMContext& ctx, quxlang::type_symbol typ)
{
    if (typ.template type_is< quxlang::int_type >())
    {
        return get_llvm_int_type_ptr(ctx, as< quxlang::int_type >(typ));
    }
    else if (quxlang::is_ref(typ))
    {
        return get_llvm_type_opaque_ptr(ctx);
    }
    else
    {
        throw std::logic_error("unimplemented");
    }
}

llvm::IntegerType* quxlang::llvm_code_generator::get_llvm_int_type_ptr(llvm::LLVMContext& context, quxlang::int_type t)
{
    assert(t.bits != 0);
    return llvm::IntegerType::get(context, t.bits);
}

llvm::PointerType* quxlang::llvm_code_generator::get_llvm_type_opaque_ptr(llvm::LLVMContext& context)
{
    return llvm::PointerType::get(context, 0);
}

bool quxlang::llvm_code_generator::generate_code(llvm::LLVMContext& context, llvm::BasicBlock*& p_block, quxlang::vm_block const& block, quxlang::vm_llvm_frame& frame, quxlang::vm_procedure& vmf)
{

    for (vm_executable_unit const& ex : block.code)
    {
        if (typeis< vm_block >(ex))
        {
            // TODO: consider adding separate frame info here
            vm_block const& block2 = as< vm_block >(ex);
            if (!generate_code(context, p_block, block2, frame, vmf))
            {
                return false;
            }
        }
        else if (ex.template type_is< vm_allocate_storage >())
        {
            assert(false);
        }
        else if (ex.template type_is< vm_store >())
        {
            llvm::IRBuilder<> builder(p_block);
            vm_store const& store = as< vm_store >(ex);

            llvm::Value* where = get_llvm_value(context, builder, frame, store.where);
            llvm::Value* what = get_llvm_value(context, builder, frame, store.what);

            llvm::Align align = llvm::Align(vm_type_alignment(store.type));
            builder.CreateAlignedStore(what, where, align);
        }
        else if (ex.template type_is< vm_return >())
        {

            llvm::IRBuilder<> builder(p_block);
            builder.CreateRetVoid();
            return false;
        }
        else if (typeis< vm_execute_expression >(ex))
        {
            vm_execute_expression const& expr = as< vm_execute_expression >(ex);
            llvm::IRBuilder<> builder(p_block);
            get_llvm_value(context, builder, frame, expr.expr);
        }
        else if (typeis< vm_if >(ex))
        {
            // std::cout << "generate if " << p_block << std::endl;
            vm_if const& if_ = as< vm_if >(ex);
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
            vm_while const& while_ = as< vm_while >(ex);
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
        else if (typeis< vm_invoke >(ex))
        {
            quxlang::vm_invoke const& call = as< quxlang::vm_invoke >(ex);
            llvm::IRBuilder<> builder(p_block);

            std::string call_expr_str = to_string(call);

            std::vector< llvm::Value* > args;
            for (auto arg : call.arguments.named)
            {
                args.push_back(get_llvm_value(context, builder, frame, arg.second));
            }
            for (auto arg : call.arguments.positional)
            {
                args.push_back(get_llvm_value(context, builder, frame, arg));
            }

            QUXLANG_DEBUG({ std::cout << "mangled name: " << call.mangled_procedure_name << std::endl; });

            llvm::FunctionType* funcType = get_llvm_type_from_func_interface(context, call.interface);
            llvm::Function* externalFunction = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, call.mangled_procedure_name, frame.module.get());

            externalFunction->setDSOLocal(true);

            // TODO: Add stack unwinding support
            return builder.CreateCall(externalFunction, args);
        }

        else
        {
            rpnx::unimplemented();
        }
    }

    return true;
}

llvm::Type* quxlang::llvm_code_generator::get_llvm_type_from_vm_storage(llvm::LLVMContext& context, vm_allocate_storage typ)
{
    // Just create an array that contains enough bytes

    return llvm::ArrayType::get(llvm::IntegerType::get(context, 8), typ.size);
}

void quxlang::llvm_code_generator::generate_arg_push(llvm::LLVMContext& context, llvm::BasicBlock* p_block, llvm::Function* p_function, quxlang::vm_procedure procedure, quxlang::vm_llvm_frame& frame)
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

    for (auto const& [name, arg_type] : procedure.interface.argument_types.named)
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

    for (auto arg_type : procedure.interface.argument_types.positional)
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

llvm::Value* quxlang::llvm_code_generator::get_llvm_value(llvm::LLVMContext& context, llvm::IRBuilder<>& builder, quxlang::vm_llvm_frame& frame, quxlang::vm_value const& value)
{

    // std::cout << "Gen llvm value for: " << quxlang::to_string(value) << std::endl;

    if (value.template type_is< quxlang::vm_expr_load_reference >())
    {
        quxlang::vm_expr_load_reference lea = as< quxlang::vm_expr_load_reference >(value);
        llvm::Value* addr = frame.values.at(lea.index).get_address;
        assert(addr != nullptr);
        return addr;
    }
    else if (value.template type_is< quxlang::vm_expr_dereference >())
    {
        quxlang::vm_expr_dereference const& deref = as< quxlang::vm_expr_dereference >(value);
        llvm::Type* load_type = get_llvm_type_from_vm_type(context, deref.type);

        llvm::Value* ptr = get_llvm_value(context, builder, frame, deref.expr);
        llvm::Value* load = builder.CreateAlignedLoad(load_type, ptr, llvm::Align(vm_type_alignment(deref.type)));

        return load;
    }
    else if (value.template type_is< quxlang::vm_expr_primitive_binary_op >())
    {
        quxlang::vm_expr_primitive_binary_op const& binop = as< quxlang::vm_expr_primitive_binary_op >(value);

        llvm::Value* lhs = get_llvm_value(context, builder, frame, binop.lhs);
        llvm::Value* rhs = get_llvm_value(context, builder, frame, binop.rhs);

        llvm::Value* result = nullptr;

        auto lhs_vm_type = vm_value_type(binop.lhs);

        auto rhs_vm_type = vm_value_type(binop.rhs);

        bool is_bool_op = false;
        bool is_int_op = false;

        std::optional< std::size_t > rhs_width;
        std::optional< std::size_t > lhs_width;
        std::optional< bool > lhs_signed;
        std::optional< bool > rhs_signed;

        if (typeis< bool_type >(lhs_vm_type) && typeis< bool_type >(rhs_vm_type))
        {
            is_bool_op = true;
        }
        else if (typeis< int_type >(lhs_vm_type) && typeis< int_type >(rhs_vm_type))
        {
            is_int_op = true;
            lhs_width = as< int_type >(lhs_vm_type).bits;
            rhs_width = as< int_type >(rhs_vm_type).bits;
            lhs_signed = as< int_type >(lhs_vm_type).has_sign;
            rhs_signed = as< int_type >(rhs_vm_type).has_sign;
        }

        std::string lhs_type_str = to_string(lhs_vm_type);
        std::string rhs_type_str = to_string(rhs_vm_type);

        std::string expr_string = quxlang::to_string(binop);

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
            if (is_int_op)
            {
                if (*rhs_signed && *lhs_signed)
                {
                    result = builder.CreateICmpSGT(lhs, rhs);
                }
                else if (!*rhs_signed && !*lhs_signed)
                {
                    result = builder.CreateICmpUGT(lhs, rhs);
                }
                else
                {
                    if (*rhs_signed)
                    {
                        auto rhs_negative = builder.CreateICmpSLT(rhs, llvm::ConstantInt::get(rhs->getType(), 0));
                        auto comparison = builder.CreateICmpUGT(lhs, rhs);
                        result = builder.CreateOr(rhs_negative, comparison);
                    }
                    else
                    {
                        assert(*lhs_signed);
                        auto lhs_nonnegative = builder.CreateICmpSGE(lhs, llvm::ConstantInt::get(lhs->getType(), 0));
                        auto comparison = builder.CreateICmpUGT(lhs, rhs);
                        result = builder.CreateAnd(lhs_nonnegative, comparison);
                    }
                }
            }
            else
            {
                throw std::logic_error("Cannot compare bools for magnituide");
            }
            return result;
        }
        else if (binop.oper == "<")
        {
            result = builder.CreateICmpSLT(lhs, rhs);
            return result;
        }
        else if (binop.oper == "<=")
        {
            result = builder.CreateICmpSLE(lhs, rhs);
        }

        assert(false);
    }
    else if (typeis< vm_expr_store >(value))
    {
        quxlang::vm_expr_store const& store = as< quxlang::vm_expr_store >(value);
        llvm::Value* lhs = get_llvm_value(context, builder, frame, store.where);
        llvm::Value* rhs = get_llvm_value(context, builder, frame, store.what);
        llvm::Value* result = nullptr;
        auto underlying_type = remove_ref(store.type);
        result = builder.CreateAlignedStore(rhs, lhs, llvm::Align(vm_type_alignment(underlying_type)));
        return result;
    }
    else if (typeis< vm_expr_load_literal >(value))
    {
        vm_expr_load_literal lit = as< vm_expr_load_literal >(value);

        if (lit.type.template type_is< int_type >())
        {
            int_type v_int_type = as< int_type >(lit.type);
            llvm::Type* llvm_int_type = get_llvm_int_type_ptr(context, v_int_type);
            std::string value_string = lit.literal;
            std::uint64_t value_number = std::stoull(value_string);
            return llvm::ConstantInt::get(llvm_int_type, value_number, v_int_type.has_sign);
        }
        if (lit.type.template type_is< pointer_type >())
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
        vm_expr_access_field const& field = as< vm_expr_access_field >(value);

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
        vm_expr_reinterpret const& reinterp = as< vm_expr_reinterpret >(value);

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

llvm::FunctionType* quxlang::llvm_code_generator::get_llvm_type_from_func_symbol(llvm::LLVMContext& context, quxlang::type_symbol typ)
{
    return nullptr;
}

llvm::FunctionType* quxlang::llvm_code_generator::get_llvm_type_from_func_interface(llvm::LLVMContext& context, quxlang::vm_procedure_interface ifc)
{
    std::vector< llvm::Type* > arg_types;
    llvm::Type* return_type{};

    // TODO: Support non-void llvm return types for types which are trivially relocatable.

    if (ifc.return_type.has_value())
    {
        return_type = get_llvm_type_from_vm_type(context, ifc.return_type.value());
        auto ptr_to_ret_type = llvm::PointerType::get(return_type, 0);
        arg_types.push_back(ptr_to_ret_type);
    }

    for (auto [name, arg_type] : ifc.argument_types.named)
    {
        arg_types.push_back(get_llvm_type_from_vm_type(context, arg_type));
    }
    for (auto arg_type : ifc.argument_types.positional)
    {
        arg_types.push_back(get_llvm_type_from_vm_type(context, arg_type));
    }

    return llvm::FunctionType::get(llvm::Type::getVoidTy(context), arg_types, false);
}

llvm::Type* quxlang::llvm_code_generator::get_llvm_intptr(llvm::LLVMContext& context)
{
    // TODO: check the actual bitwidth on the current platform
    return llvm::IntegerType::get(context, 64);
}

std::vector< std::byte > quxlang::llvm_code_generator::assemble(quxlang::asm_procedure input)
{
    std::cout << "Target triple is: " << target_triple_str << std::endl;
    std::string assembly;

    if (m_machine_info.cpu_type == quxlang::cpu::arm_32 || m_machine_info.cpu_type == cpu::arm_64)
    {
        assembly = quxlang::convert_to_arm_asm(input.instructions.begin(), input.instructions.end(), input.name);
    }
    else
    {
        throw std::logic_error("Unsupported CPU type");
    }

    llvm::SmallVector< char, 16 > output;
    // auto target_triple_str = llvm::Triple("aarch64-none-linux-gnu");
    std::string Error;

    llvm::SourceMgr source_manager;

    source_manager.AddNewSourceBuffer(llvm::MemoryBuffer::getMemBufferCopy(assembly), llvm::SMLoc());

    llvm::MCTargetOptions mc_options;
    std::unique_ptr< llvm::MCSubtargetInfo > subtarget_info(target->createMCSubtargetInfo(target_triple_str, "generic", ""));
    std::unique_ptr< llvm::MCRegisterInfo > machine_register_info(target->createMCRegInfo(target_triple_str));
    auto triple = llvm::Triple(target_triple_str);

    std::unique_ptr< llvm::MCAsmInfo > machine_asm_info(target->createMCAsmInfo(*machine_register_info, target_triple_str, mc_options));

    llvm::MCContext machine_context(triple, machine_asm_info.get(), machine_register_info.get(), subtarget_info.get(), &source_manager, &mc_options);
    std::unique_ptr< llvm::MCObjectFileInfo > machine_object_file_info(target->createMCObjectFileInfo(machine_context, false, true));
    machine_context.setObjectFileInfo(machine_object_file_info.get());

    std::unique_ptr< llvm::MCAsmBackend > machine_code_asm_backend(target->createMCAsmBackend(*subtarget_info, *machine_register_info, mc_options));

    auto output_stream = std::make_unique< llvm::raw_svector_ostream >(output);
    std::unique_ptr< llvm::MCObjectWriter > object_writer(machine_code_asm_backend->createObjectWriter(*output_stream));

    std::unique_ptr< llvm::MCInstrInfo > machine_code_instruction_info(target->createMCInstrInfo());
    std::unique_ptr< llvm::MCCodeEmitter > machine_code_emitter(target->createMCCodeEmitter(*machine_code_instruction_info, machine_context));
    std::unique_ptr< llvm::MCStreamer > machine_code_streamer(target->createMCObjectStreamer(llvm::Triple(target_triple_str), machine_context, std::move(machine_code_asm_backend), std::move(object_writer), std::move(machine_code_emitter), *subtarget_info, false, false, false));

    llvm::MCAsmParser* asm_parser = llvm::createMCAsmParser(source_manager, machine_context, *machine_code_streamer, *machine_asm_info);
    std::unique_ptr< llvm::MCTargetAsmParser > target_asm_parser(target->createMCAsmParser(*subtarget_info, *asm_parser, *machine_code_instruction_info, mc_options));

    if (!target_asm_parser)
    {
        throw std::logic_error("Failed to create target ASM parser!");
    }

    asm_parser->setTargetParser(*target_asm_parser.get());

    if (asm_parser->Run(false))
    {
        throw std::logic_error("Assembly parsing failed!\n");
    }

    std::ofstream output_file(input.name + ".o", std::ios::out | std::ios::binary | std::ios::trunc);

    output_file.write(output.data(), output.size());

    return {};
}

quxlang::llvm_code_generator::llvm_code_generator(quxlang::output_info m)
    : m_machine_info(m)
{
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    target_triple_str = lookup_llvm_triple(m_machine_info);

    std::string err;

    target = llvm::TargetRegistry::lookupTarget(target_triple_str, err);

    if (!target)
    {
        throw std::logic_error("Failed to lookup target: " + err);
    }

    auto CPU = "generic";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional< llvm::Reloc::Model >();
    RM = llvm::Reloc::Model::Static;

    llvm::Optional< llvm::CodeModel::Model > code_model;
    if (m_machine_info.cpu_type == quxlang::cpu::arm_64 || m_machine_info.cpu_type == quxlang::cpu::x86_64 || m_machine_info.cpu_type == quxlang::cpu::riscv_64)
    {
        code_model = llvm::CodeModel::Large;
    }
    else
    {
        code_model = llvm::CodeModel::Medium;
    }

    opt.ExceptionModel = llvm::ExceptionHandling::DwarfCFI;
    target_machine = target->createTargetMachine(target_triple_str, CPU, "", opt, RM, code_model);
}

std::unique_ptr< llvm::MemoryBuffer > to_llvm_buffer(std::string str)
{
    return llvm::MemoryBuffer::getMemBufferCopy(str);
}

void quxlang::llvm_code_generator::foo()
{
    std::cout << "Target triple is: " << target_triple_str << std::endl;
    std::string assembly(R"(
        .text
        .global _start
        _start:
            movz x1, 1
            movz x7, 1
            svc 0
        )");
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    llvm::SmallVector< char, 16 > output;
    // auto target_triple_str = llvm::Triple("aarch64-none-linux-gnu");
    std::string Error;

    llvm::SourceMgr source_manager;

    source_manager.AddNewSourceBuffer(llvm::MemoryBuffer::getMemBufferCopy(assembly), llvm::SMLoc());

    llvm::MCTargetOptions mc_options;
    std::unique_ptr< llvm::MCSubtargetInfo > subtarget_info(target->createMCSubtargetInfo(target_triple_str, "generic", ""));
    std::unique_ptr< llvm::MCRegisterInfo > machine_register_info(target->createMCRegInfo(target_triple_str));
    auto triple = llvm::Triple(target_triple_str);

    std::unique_ptr< llvm::MCAsmInfo > machine_asm_info(target->createMCAsmInfo(*machine_register_info, target_triple_str, mc_options));

    llvm::MCContext machine_context(triple, machine_asm_info.get(), machine_register_info.get(), subtarget_info.get(), &source_manager, &mc_options);
    std::unique_ptr< llvm::MCObjectFileInfo > machine_object_file_info(target->createMCObjectFileInfo(machine_context, false, true));
    machine_context.setObjectFileInfo(machine_object_file_info.get());

    std::unique_ptr< llvm::MCAsmBackend > machine_code_asm_backend(target->createMCAsmBackend(*subtarget_info, *machine_register_info, mc_options));

    auto output_stream = std::make_unique< llvm::raw_svector_ostream >(output);
    std::unique_ptr< llvm::MCObjectWriter > object_writer(machine_code_asm_backend->createObjectWriter(*output_stream));

    std::unique_ptr< llvm::MCInstrInfo > machine_code_instruction_info(target->createMCInstrInfo());
    std::unique_ptr< llvm::MCCodeEmitter > machine_code_emitter(target->createMCCodeEmitter(*machine_code_instruction_info, machine_context));
    std::unique_ptr< llvm::MCStreamer > machine_code_streamer(target->createMCObjectStreamer(llvm::Triple(target_triple_str), machine_context, std::move(machine_code_asm_backend), std::move(object_writer), std::move(machine_code_emitter), *subtarget_info, false, false, false));

    llvm::MCAsmParser* asm_parser = llvm::createMCAsmParser(source_manager, machine_context, *machine_code_streamer, *machine_asm_info);
    std::unique_ptr< llvm::MCTargetAsmParser > target_asm_parser(target->createMCAsmParser(*subtarget_info, *asm_parser, *machine_code_instruction_info, mc_options));

    if (!target_asm_parser)
    {
        llvm::errs() << "Failed to create target ASM parser!\n";
        return;
    }

    asm_parser->setTargetParser(*target_asm_parser.get());

    if (asm_parser->Run(false))
    {
        llvm::errs() << "Assembly parsing failed!\n";
    }

    std::ofstream output_file("output.o", std::ios::out | std::ios::binary | std::ios::trunc);

    output_file.write(output.data(), output.size());
}

static std::unique_ptr< llvm::Module > parse_llvm_bitcode(llvm::LLVMContext& llvm_ctx, const std::vector< std::byte >& ir)
{
    llvm::StringRef ir_data(reinterpret_cast< const char* >(ir.data()), ir.size());
    auto mem_buffer = llvm::MemoryBuffer::getMemBuffer(ir_data, "", false);

    llvm::Expected< std::unique_ptr< llvm::Module > > module_or_e = llvm::parseBitcodeFile(mem_buffer->getMemBufferRef(), llvm_ctx);

    if (module_or_e)
    {
        return std::move(module_or_e.get());
    }
    else
    {
        throw std::logic_error(llvm::toString(module_or_e.takeError()));
    }
}

std::vector< std::byte > quxlang::llvm_code_generator::compile_llvm_ir_to_elf(std::vector< std::byte > ir)
{
    llvm::LLVMContext llvm_ctx;
    auto module = parse_llvm_bitcode(llvm_ctx, ir);

    llvm::SmallVector< char, 0 > mc_buffer;
    llvm::raw_svector_ostream mc_buffer_stream(mc_buffer);

    llvm::legacy::PassManager pm;

    auto result = target_machine->addPassesToEmitFile(pm, mc_buffer_stream, nullptr, llvm::CGFT_ObjectFile);

    if (result)
    {
        std::cerr << "Failed to emit object file" << std::endl;
        return {};
    }
    pm.run(*module);

    // std::cout << "Object code for " << vmf.mangled_name << std::endl;
    std::cout << mc_buffer.size() << " bytes" << std::endl;

    std::vector< std::byte > bytecode_vector;

    for (char c : mc_buffer)
    {
        bytecode_vector.push_back(std::byte(c));
    }

    return bytecode_vector;
}
