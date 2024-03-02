//
// Created by Ryan Nicholl on 9/11/23.
//

//
// Created by Ryan Nicholl on 7/20/23.
//
#include "rylang/compiler.hpp"

namespace rylang
{
    void module_ast_precursor1_resolver::process(compiler* c)
    {
        auto files_in_module_dep = get_dependency(
            [&]
            {
                return c->lk_files_in_module(get_input());
            });

        if (!ready())
            return;

        auto files_in_module = files_in_module_dep->get();

        module_ast_precursor1 result;
        result.module_name = get_input();

        for (auto& file : files_in_module)
        {
            auto file_ast_dep = get_dependency(
                [&]
                {
                    return c->lk_file_ast(file);
                });

            if (!ready())
                return;

            auto file_ast = file_ast_dep->get();

            result.files.push_back(file_ast);
        }

        set_value(result);
    }
} // namespace rylang