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
rylang::llvm_proxy_type rylang::compiler::get_llvm_proxy_return_type_of(qualified_symbol_reference chain)
{
    // TODO: Implement this with real code
    return rylang::llvm_proxy_type_int{32, false};
}
std::vector< rylang::llvm_proxy_type > rylang::compiler::get_llvm_proxy_argument_types_of(rylang::qualified_symbol_reference chain)
{
    // TODO: Make this use real values
    return {llvm_proxy_type_pointer(), llvm_proxy_type_int{32, false}};
}

rylang::function_ast rylang::compiler::get_function_ast_of_overload(qualified_symbol_reference chain)
{
    return rylang::function_ast{};
}
rylang::call_parameter_information rylang::compiler::get_function_overload_selection(qualified_symbol_reference chain, call_parameter_information args)

{
    auto node = lk_function_overload_selection(chain, args);
    m_solver.solve(this, node);
    return node->get();
}
