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

rylang::compiler::out<rylang::file_ast> rylang::compiler::lk_file_ast(std::string const& filename)
{
    return m_file_ast_index.lookup(filename);
}
