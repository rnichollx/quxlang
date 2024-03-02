#include "quxlang/compiler.hpp"

quxlang::compiler::compiler(int argc, char** argv, output_info target_machine)
    : m_machine_info(target_machine)
{
    for (int i = 1; i < argc; i++)
    {
        m_file_list.push_back(argv[i]);
    }
}

rpnx::output_ptr< quxlang::compiler, std::string > quxlang::compiler::file_contents(std::string const& filename)
{
    return m_file_contents_index.lookup(filename);
}

quxlang::compiler::out< quxlang::ast2_file_declaration > quxlang::compiler::lk_file_ast(std::string const& filename)
{
    return m_file_ast_index.lookup(filename);
}



quxlang::function_ast quxlang::compiler::get_function_ast_of_overload(type_symbol chain)
{
    return quxlang::function_ast{};
}
quxlang::call_parameter_information quxlang::compiler::get_function_overload_selection(type_symbol chain, call_parameter_information args)

{
    auto node = lk_function_overload_selection(chain, args);
    m_solver.solve(this, node);
    return node->get();
}

quxlang::type_symbol quxlang::compiler::get_function_qualname(quxlang::type_symbol name, quxlang::call_parameter_information args)
{
    auto node = lk_function_qualname(name, args);
    m_solver.solve(this, node);
    return node->get();
}
