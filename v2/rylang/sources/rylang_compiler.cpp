#include "rylang/compiler.hpp"

rylang::compiler::compiler(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        m_file_list.push_back(argv[i]);
    }
}

rylang::filelist rylang::compiler::get_file_list()
{
    return m_file_list;
}

rpnx::output_ptr< rylang::compiler, std::string > rylang::compiler::file_contents(std::string const& filename)
{
    return m_file_contents_index.lookup(filename);
}
