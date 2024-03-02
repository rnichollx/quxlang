//
// Created by Ryan Nicholl on 7/20/23.
//

#include "quxlang/compiler.hpp"

namespace quxlang
{
    void filelist_resolver::process(compiler* c)
    {
        set_value(c->m_file_list);
    }
} // namespace quxlang