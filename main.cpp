#include <iostream>
//#include "llvm/ADT/APFloat.h"
//#include "llvm/ADT/STLExtras.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <boost/any.hpp>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <typeindex>


template <typename T>
auto GenerateLLVMCode(llvm::IRBuilder<> & builder, llvm::Module & mod, T const & obj) -> llvm::unique_value;

struct llvm_runtime_bindings
{
    std::recursive_mutex m;
    std::map<std::type_index, std::function<llvm::unique_value(llvm::IRBuilder<> &, llvm::Module &, boost::any const&)>> m_map;

    static llvm_runtime_bindings& global()
    {
        static llvm_runtime_bindings instance;
        return instance;
    }
public:

    template <typename T>
    static void bind()
    {
        global().lbind<T>();
    }

    static llvm::unique_value generate_code(llvm::IRBuilder<> & b, llvm::Module  &mod,  boost::any const & obj)
    {
        return global().lgenerate_code(b, mod, obj);
    }
private:
    llvm::unique_value lgenerate_code(llvm::IRBuilder<> & b, llvm::Module  &mod,  boost::any const & obj)
    {
        std::recursive_mutex &m_mutex_map = m;
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

    template <typename T>
    void lbind()
    {
        std::unique_lock lock(m);
        m_map[std::type_index(typeid(T))] = [](llvm::IRBuilder<>  & builder, llvm::Module & mod, boost::any const & obj) -> llvm::unique_value
        {
            return GenerateLLVMCode(builder, mod, boost::any_cast<T const &>(obj));
        };
    }
};

template <typename T>
struct llvm_runtime_binder
{
public:
    llvm_runtime_binder()
    {
        llvm_runtime_bindings::bind<T>();
    }
};

struct operation_load_constant_i32 {
    std::int32_t value;
};

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<> & , llvm::Module & module, operation_load_constant_i32 const & obj)
{
    // load the constant
    llvm::Value * v = llvm::ConstantInt::get(module.getContext(), llvm::APInt(32, obj.value));
    return llvm::unique_value(v);
}

struct operation_accumulate {
    std::vector<boost::any> values;
};

struct operation_return {
    boost::any value;
};



struct operation_load_argument_by_index {
    std::size_t index;
};

struct operation_load_argument_by_name {
    std::string name;
};

struct operation_load_argument_ref_by_name {
    std::string name;
};

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<> & builder, llvm::Module & module, operation_return const & obj)
{
    llvm::Value * v = llvm_runtime_bindings::generate_code(builder, module, obj.value).release();
    llvm::Value * r = builder.CreateRet(v);
    return llvm::unique_value(r);
}

template <>
llvm::unique_value GenerateLLVMCode(llvm::IRBuilder<>& builder,
                                    llvm::Module & mod,  operation_accumulate const & obj)
{
    llvm::unique_value result = nullptr;
    for (auto const & v : obj.values)
    {
        if (result == nullptr)
        {
            result = llvm_runtime_bindings::generate_code(builder, mod, v);
        }
        // Iterate over the rest of the values, adding each one to the result
        else
        {
            llvm::Value * lhs = result.release();
            llvm::Value * rhs = llvm_runtime_bindings::generate_code(builder, mod, v).release();
            llvm::Value * result_v = builder.CreateAdd(lhs, rhs);
            result = llvm::unique_value(result_v);
        }
    }
    return result;
}


llvm_runtime_binder<operation_accumulate> operation_accumulate_bindings;

llvm_runtime_binder<operation_load_constant_i32> operation_load_constant_i32_bindings;






int main() {
    std::cout << "Hello, World!" << std::endl;


    llvm::LLVMContext ctx;
    llvm::Module module("test", ctx);
    llvm::IRBuilder<> builder(ctx);

    // Take 4 ints and add them together
    llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), {builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty(), builder.getInt32Ty()}, false);

    llvm::Function *func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "add", module);

    // Create a block to start insertion into.

    llvm::BasicBlock *block = llvm::BasicBlock::Create(ctx, "entry", func);
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
