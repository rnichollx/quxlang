#include "rylang/compiler.hpp"

rylang::compiler::compiler(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        m_file_list.push_back(argv[i]);
    }
}

rylang::filelist rylang::compiler::get_file_list()
{
    // TODO: return m_file_list
    return rylang::filelist{"foo.ry", "bar.ry"};
}
