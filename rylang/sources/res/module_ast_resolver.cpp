//
// Created by Ryan Nicholl on 9/20/23.
//

//
// Created by Ryan Nicholl on 9/11/23.
//

#include "rylang/compiler.hpp"
#include "rylang/manipulators/merge_entity.hpp"
#include "rylang/res/module_ast_precursor1_resolver.hpp"

#include <rylang/ast2/ast2_entity.hpp>
#include <rylang/ast2/ast2_module.hpp>

namespace rylang
{
    void module_ast_resolver::process(compiler* c)
    {
        auto files_in_module_dep = get_dependency(
            [&]
            {
                return c->lk_files_in_module(m_id);
            });

        if (!ready())
            return;

        auto files_in_module = files_in_module_dep->get();

        ast2_module_declaration result;

        for (auto& file : files_in_module)
        {
            auto file_ast_dep = get_dependency(
                [&]
                {
                    return c->lk_file_ast(file);
                });

            if (!ready())
                return;

            ast2_file_declaration file_ast = file_ast_dep->get();

            // TODO: Check for duplicate imports
            for (auto import : file_ast.imports)
            {
                result.imports.insert(import);
            }

            for (auto global : file_ast.globals)
            {
                result.globals.push_back(global);
            }

        }

        set_value(result);

    }
} // namespace rylang