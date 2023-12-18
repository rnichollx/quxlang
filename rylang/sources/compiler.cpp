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

rylang::compiler::out< rylang::ast2_file_declaration > rylang::compiler::lk_file_ast(std::string const& filename)
{
    return m_file_ast_index.lookup(filename);
}



rylang::function_ast rylang::compiler::get_function_ast_of_overload(type_symbol chain)
{
    return rylang::function_ast{};
}
rylang::call_parameter_information rylang::compiler::get_function_overload_selection(type_symbol chain, call_parameter_information args)

{
    auto node = lk_function_overload_selection(chain, args);
    m_solver.solve(this, node);
    return node->get();
}
rylang::type_symbol rylang::compiler::get_function_qualname(rylang::type_symbol name, rylang::call_parameter_information args)
{
    auto node = lk_function_qualname(name, args);
    m_solver.solve(this, node);
    return node->get();
}
