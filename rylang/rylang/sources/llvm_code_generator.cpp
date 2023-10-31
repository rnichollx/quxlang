//
// Created by Ryan Nicholl on 10/28/23.
//
#include "rylang/llvm_code_generator.hpp"
#include "rylang/compiler.hpp"
#include "rylang/data/llvm_proxy_types.hpp"

std::vector< std::byte > rylang::llvm_code_generator::get_function_code(cpu_arch cpu_type, canonical_resolved_function_chain ch)
{
    // TODO: support multiple CPU architectures
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder(context);

    llvm_proxy_type func_return_type = c->get_llvm_proxy_return_type_of(ch);

    llvm::Type* func_llvm_return_type = get_llvm_type_from_proxy(context, func_return_type);

    std::vector< llvm::Type* > func_llvm_arg_types;

    auto arg_types = c->get_llvm_proxy_argument_types_of(ch);

    for (auto arg_type : arg_types)
    {
        func_llvm_arg_types.push_back(get_llvm_type_from_proxy(context, arg_type));
    }

    llvm::Function* func = llvm::Function::Create(llvm::FunctionType::get(func_llvm_return_type, func_llvm_arg_types, false), llvm::Function::ExternalLinkage, "main", nullptr);

    llvm::BasicBlock* entry = llvm::BasicBlock::Create(context, "entry", func);

    rylang::function_ast func_ast = c->get_function_ast_of_overload(ch);

    return {};
}

llvm::Type* rylang::llvm_code_generator::get_llvm_type_from_proxy(llvm::LLVMContext& ctx, rylang::llvm_proxy_type typ)
{
    if (typ.type() == boost::typeindex::type_id< rylang::llvm_proxy_type_int >())
    {
        return get_llvm_int_type_ptr(ctx, boost::get< rylang::llvm_proxy_type_int >(typ));
    }
    else if (typ.type() == boost::typeindex::type_id< rylang::llvm_proxy_type_pointer >())
    {
        return get_llvm_type_opaque_ptr(ctx);
    }
    else
    {
        throw std::runtime_error("Unknown type");
    }
}

llvm::IntegerType* rylang::llvm_code_generator::get_llvm_int_type_ptr(llvm::LLVMContext& context, rylang::llvm_proxy_type_int t)
{
    return llvm::IntegerType::get(context, t.bits);
}
llvm::PointerType* rylang::llvm_code_generator::get_llvm_type_opaque_ptr(llvm::LLVMContext& context)
{
    return llvm::PointerType::get(context, 0);
}
