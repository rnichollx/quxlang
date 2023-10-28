#include "rylang/compiler.hpp"

rylang::compiler::compiler(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        m_file_list.push_back(argv[i]);
    }
}

rpnx::output_ptr< rylang::compiler, std::string > rylang::compiler::file_contents(std::string const& filename)
{
    return m_file_contents_index.lookup(filename);
}

rylang::compiler::out< rylang::file_ast > rylang::compiler::lk_file_ast(std::string const& filename)
{
    return m_file_ast_index.lookup(filename);
}
rylang::llvm_proxy_type rylang::compiler::get_llvm_proxy_return_type_of(rylang::canonical_resolved_function_chain chain)
{
    return rylang::llvm_proxy_type_int{32, false};
}
std::vector< rylang::llvm_proxy_type > rylang::compiler::get_llvm_proxy_argument_types_of(rylang::canonical_resolved_function_chain chain)
{
    return {llvm_proxy_type_pointer(), llvm_proxy_type_int{32, false}};
}
