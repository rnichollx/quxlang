//
// Created by Ryan Nicholl on 7/21/23.
//

#include "rylang/res/class_list_resolver.hpp"
#include "rylang/compiler.hpp"

void rylang::class_list_resolver::process(compiler* c)
{

    auto files = get_dependency(
        [&]
        {
            return &c->m_filelist_resolver;
        });

    if (!ready())
        return;

    auto files_v = files->get();

    class_list result;

    for (std::string filename : files_v)
    {
        auto ast = get_dependency(
            [&]
            {
                return c->lk_file_ast(filename);
            });

        if (!ready())
            return;

        auto per_file_class_list_dp = ast->get();

        for (auto kv : per_file_class_list_dp.root.m_sub_entities)
        {
            if (kv.second.get().m_category != entity_category::class_cat)
                continue;
            result.class_names.push_back(kv.first);
        }
    }

    set_value(result);
}
