#include "rylang/compiler.hpp"

rylang::compiler::compiler()
{
}

rylang::filelist rylang::compiler::get_file_list()
{
    return rylang::filelist{"foo.ry", "bar.ry"};
}


