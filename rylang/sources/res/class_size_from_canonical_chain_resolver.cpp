//
// Created by Ryan Nicholl on 10/20/23.
//
#include "rylang/res/class_size_from_canonical_chain_resolver.hpp"

#include "rylang/compiler.hpp"

#include <iostream>
void rylang::class_size_from_canonical_chain_resolver::process(compiler* c)
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
