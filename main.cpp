#include <iostream>
// #include "llvm/ADT/APFloat.h"
// #include "llvm/ADT/STLExtras.h"

#include "argparser.hpp"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/TargetSelect.h"
#include <boost/any.hpp>
#include <filesystem>
#include <fstream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <typeindex>

#include "collector.hpp"
#include "compiler.hpp"
#include "parser.hpp"
#include "semantic_generator.hpp"

template < typename T >
auto GenerateLLVMCode(llvm::IRBuilder<>& builder, llvm::Module& mod, T const& obj) -> llvm::unique_value;

struct llvm_runtime_bindings
{
    std::recursive_mutex m;
    std::map< std::type_index, std::function< llvm::unique_value(llvm::IRBuilder<>&, llvm::Module&, boost::any const&) > > m_map;

    static llvm_runtime_bindings& global()
    {
        static llvm_runtime_bindings instance;
        return instance;
    }

  public:
    template < typename T >
    static void bind()
    {
        global().lbind< T >();
    }

    static llvm::unique_value generate_code(llvm::IRBuilder<>& b, llvm::Module& mod, boost::any const& obj)
    {
        return global().lgenerate_code(b, mod, obj);
    }

  private:
    llvm::unique_value lgenerate_code(llvm::IRBuilder<>& b, llvm::Module& mod, boost::any const& obj)
    {
        std::recursive_mutex& m_mutex_map = m;
        std::unique_lock lock(m_mutex_map);
        auto it = m_map.find(obj.type());
        if (it == m_map.end())
        {
            // print typename demangled
            std::string demangled_typename = abi::__cxa_demangle(obj.type().name(), nullptr, nullptr, nullptr);
            std::cerr << "No binding for type " << demangled_typename << std::endl;
            throw std::runtime_error("No code generator for type");
        }

        return it->second(b, mod, obj);
    }

    template < typename T >
    void lbind()
    {
        std::unique_lock lock(m);
        m_map[std::type_index(typeid(T))] = [](llvm::IRBuilder<>& builder, llvm::Module& mod, boost::any const& obj) -> llvm::unique_value
        {
            return GenerateLLVMCode(builder, mod, boost::any_cast< T const& >(obj));
        };
    }
};

template < typename T >
struct llvm_runtime_binder
{
  public:
    llvm_runtime_binder()
    {
        llvm_runtime_bindings::bind< T >();
    }
};

struct operation_load_constant_i32
{
    std::int32_t value;
};

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<>&, llvm::Module& module, operation_load_constant_i32 const& obj)
{
    // load the constant
    llvm::Value* v = llvm::ConstantInt::get(module.getContext(), llvm::APInt(32, obj.value));
    return llvm::unique_value(v);
}

struct operation_accumulate
{
    std::vector< boost::any > values;
};

struct operation_return
{
    boost::any value;
};

struct operation_load_argument_by_index
{
    std::size_t index;
};

struct operation_load_argument_by_name
{
    std::string name;
};

struct operation_load_argument_ref_by_name
{
    std::string name;
};

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<>& builder, llvm::Module& module, operation_return const& obj)
{
    llvm::Value* v = llvm_runtime_bindings::generate_code(builder, module, obj.value).release();
    llvm::Value* r = builder.CreateRet(v);
    return llvm::unique_value(r);
}

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<>& builder, llvm::Module& mod, operation_accumulate const& obj)
{
    llvm::unique_value result = nullptr;
    for (auto const& v : obj.values)
    {
        if (result == nullptr)
        {
            result = llvm_runtime_bindings::generate_code(builder, mod, v);
        }
        // Iterate over the rest of the values, adding each one to the result
        else
        {
            llvm::Value* lhs = result.release();
            llvm::Value* rhs = llvm_runtime_bindings::generate_code(builder, mod, v).release();
            llvm::Value* result_v = builder.CreateAdd(lhs, rhs);
            result = llvm::unique_value(result_v);
        }
    }
    return result;
}

llvm_runtime_binder< operation_accumulate > operation_accumulate_bindings;

llvm_runtime_binder< operation_load_constant_i32 > operation_load_constant_i32_bindings;

std::string load_string_from_file(std::filesystem::path where)
{
    std::ifstream file(where);
    std::string str((std::istreambuf_iterator< char >(file)), std::istreambuf_iterator< char >());
    return str;
}

int main(int argc, char** argv)
{
    rs1031::argparser args(argc, argv);
    std::vector< boost::any > values;
    rs1031::collector c;
    rs1031::semantic_generator sg;

    std::filesystem::path input_path = args.get_path(1);

    std::string example = load_string_from_file(input_path);

    c.emit_class = [&sg](std::string name, rs1031::ast_class cl)
    {
        std::cout << "Emitting class " << name << std::endl;
        sg.add(std::vector< std::string >{"__main", name}, cl);
    };
    c.emit_function = [&sg](std::string name, rs1031::ast_function func)
    {
        std::cout << "Emitting function " << name << std::endl;
        std::cout << "  " << func.to_string() << std::endl;
        sg.add({"__main", name}, func);
    };
    c.collect(example.begin(), example.end());

    rs1031::lir_machine_info machine{};
    machine.m_pointer_alignment = 8;
    machine.m_pointer_size = 8;

    // rs1031::lir_type_index v_type_index(machine);

    // sg.resolve_lir(v_type_index);
    //  sg.resolve();

    // return 0;

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    std::cout << "Hello, World!" << std::endl;

    llvm::LLVMContext ctx;
    llvm::Module module("test", ctx);
    llvm::IRBuilder<> builder(ctx);

    // Take 4 ints and add them together
    llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);

    llvm::Function* func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "add", module);

    // Create a block to start insertion into.

    llvm::BasicBlock* block = llvm::BasicBlock::Create(ctx, "entry", func);
    builder.SetInsertPoint(block);

    // attach the block to the function

    // create an operation_accumulate object

    operation_accumulate op;
    for (int i = 0; i < 4; ++i)
    {
        op.values.push_back(operation_load_constant_i32{i});
    }
    // generate the code for the operation_accumulate object
    auto val = llvm_runtime_bindings::generate_code(builder, module, op);

    // Create the return instruction and add it to the basic block
    builder.CreateRet(val.release());

    // Validate the generated code, checking for consistency.
    // output to cerr
    bool error = llvm::verifyFunction(*func, &llvm::errs());

    std::cout << "Was error? " << error << std::endl;

    // print the llvm ir for the function
    func->print(llvm::outs());

    return 0;
}
