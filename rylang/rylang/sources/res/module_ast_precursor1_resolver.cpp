//
// Created by Ryan Nicholl on 9/11/23.
//

//
// Created by Ryan Nicholl on 7/20/23.
//

#include "rylang/res/module_ast_precursor1_resolver.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    void module_ast_precursor1_resolver::process(compiler* c)
    {
       auto files_in_module_dep = get_dependency([&]{
        return c->lk_files_in_module(m_id);
       });

       if (!ready()) return;

       auto files_in_module = files_in_module_dep->get();
    }
} // namespace rylang