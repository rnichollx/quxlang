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
#include <typeindex>


template <typename T>
auto GenerateLLVMCode(llvm::LLVMContext & ctx, T const & obj) -> llvm::Value*;

struct GenerateLLVMCodeMap
{
    std::mutex m;
    std::map<std::type_index, std::function<llvm::Value*(llvm::LLVMContext&, boost::any const&)>> m_map;

    static GenerateLLVMCodeMap& global()
    {
        static GenerateLLVMCodeMap instance;
        return instance;
    }
public:

    template <typename T>
    static void bind()
    {
        global().lbind<T>();
    }
private:

    template <typename T>
    void lbind()
    {
        std::unique_lock<std::mutex> lock(m);
        m_map[std::type_index(typeid(T))] = [](llvm::LLVMContext & ctx, boost::any const & obj) -> llvm::Value*
        {
            return GenerateLLVMCode(ctx, boost::any_cast<T const &>(obj));
        };
    }
};

template <typename T>
struct LLVMCodeBinding
{
public:
    LLVMCodeBinding()
    {
        GenerateLLVMCodeMap::bind<T>();
    }
};

};

struct operation_accumulate {
    std::vector<boost::any> values;
};


int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
