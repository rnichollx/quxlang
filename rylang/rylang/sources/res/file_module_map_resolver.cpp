//
// Created by Ryan Nicholl on 9/11/23.
//

#include "rylang/res/file_module_map_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::file_module_map_resolver::process(compiler* c)
{
    // Get the list of files, then map each module to the list of files in that module

        auto filelist_dep = get_dependency(
                [&]
                {
                return c->lk_file_list();
                });

        if (!ready())
                return;

        filelist fl = filelist_dep->get();
}