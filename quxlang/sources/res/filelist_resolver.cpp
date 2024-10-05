// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"

namespace quxlang
{
    void filelist_resolver::process(compiler* c)
    {
        set_value(c->m_file_list);
    }
} // namespace quxlang