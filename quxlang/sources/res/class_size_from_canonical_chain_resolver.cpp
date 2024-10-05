// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_size_from_canonical_chain_resolver.hpp"



#include <iostream>
void quxlang::class_size_from_canonical_chain_resolver::process(compiler* c)
{
    auto placement_dp = get_dependency(
        [&]
        {
            return c->lk_type_placement_info_from_canonical_type(m_chain);
        });

    if (!ready())
        return;

    type_placement_info placement = placement_dp->get();

    set_value(placement.size);
}
