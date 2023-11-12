//
// Created by Ryan Nicholl on 7/21/23.
//

#include "rylang/res/classes_per_file_resolver.hpp"
#include "rylang/compiler.hpp"

namespace rylang
{
    void classes_per_file_resolver::process(compiler* c)
    {
        class_list result;
        auto files_d = get_dependency(
            [&]
            {
                return c->lk_file_list();
            });

        if (!ready())
            return;

        auto files = files_d->get();

        for (auto& file_name : files)
        {
            if (file_name != input_filename)
                continue;

            auto ast_node = get_dependency(
                [&]
                {
                return c->lk_file_ast(file_name);
                });

            if (!ready())
                return;

            auto ast = ast_node->get();

            for (auto& kv : ast.root.m_sub_entities)
            {
                if ( ! std::holds_alternative<class_entity_ast>(kv.second.get().m_specialization.get()))
                    continue;
                result.class_names.push_back(kv.first);
            }

            set_value(result);
            return;
        }
        // TODO:
        //   set_error()

    }
} // namespace rylang