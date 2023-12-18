//
// Created by Ryan Nicholl on 9/11/23.
//

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

    file_module_map result;

    for (auto& filename : fl)
    {
        auto file_ast_dp = get_dependency(
            [&]
            {
                return c->lk_file_ast(filename);
            });

        if (!ready())
            return;

        ast2_file_declaration ast = file_ast_dp->get();

        assert(ast.module_name == "main");
        // TODO: support multiple modules

        result[ast.module_name].insert(filename);
    }

    set_value(result);
}