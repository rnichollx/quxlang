//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/res/filelist_resolver.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    void filelist_resolver::process(compiler* c)
    {
        set_value(c->m_file_list);
    }
} // namespace rylang