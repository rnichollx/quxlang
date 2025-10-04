// Copyright 2024-2025 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/manipulators/merge_entity.hpp"
#include <quxlang/compiler.hpp>
#include <quxlang/res/symboid_resolver.hpp>

QUX_CO_RESOLVER_IMPL_FUNC_DEF(symboid)
{

    if (typeis< absolute_module_reference >(input_val))
    {
        auto const & module_ref = as< absolute_module_reference >(input_val);
        co_return co_await QUX_CO_DEP(module_ast, (as< absolute_module_reference >(input_val).module_name));
    }

    auto declaroids = co_await QUX_CO_DEP(declaroids, (input_val));

    ast2_symboid output;

    for (auto& decl : declaroids)
    {
        merge_entity(output, decl);
    }

    co_return output;
}