//
// Created by Ryan Nicholl on 9/20/23.
//

//
// Created by Ryan Nicholl on 9/11/23.
//

#include "rylang/res/module_ast_resolver.hpp"
#include "rylang/compiler.hpp"
#include "rylang/res/module_ast_precursor1_resolver.hpp"

namespace rylang
{
    void module_ast_resolver::process(compiler* c)
    {
        auto precursor1_dep = get_dependency(
            [&]
            {
                return c->lk_module_ast_precursor1(m_id);
            });

        if (!ready())
            return;

        auto precursor1 = precursor1_dep->get();

        // TODO: Perform precursor transformations
        // For now, we don't do any precursor1 -> 2 transformations

        module_ast result;



    }
} // namespace rylang