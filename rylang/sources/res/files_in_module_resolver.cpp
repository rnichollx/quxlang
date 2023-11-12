//
// Created by Ryan Nicholl on 9/11/23.
//
#include "rylang/res/files_in_module_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::files_in_module_resolver::process(compiler* c)
{
    auto file_module_map_dep = get_dependency(
        [&]
        {
            return c->lk_file_module_map();
        });

    if (!ready())
        return;

    file_module_map v_file_module_map = file_module_map_dep->get();
    // TODO

    filelist result;

    for (auto& [module_id, file_list] : v_file_module_map)
    {
        if (module_id == m_id)
        {
            for (auto const& file : file_list)
            {
                result.push_back(file);
            }
        }
    }

    set_value(result);
}
