//
// Created by Ryan Nicholl on 4/22/24.
//

#include "quxlang/manipulators/merge_entity.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/res/symboid_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symboid)
{
    auto declaroids = co_await QUX_CO_DEP(declaroids, (input_val));

    ast2_symboid output;

    for (auto& decl : declaroids)
    {
        merge_entity(output, decl);
    }

    co_return output;
}