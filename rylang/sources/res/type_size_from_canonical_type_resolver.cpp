//
// Created by Ryan Nicholl on 10/20/23.
//

#include "rylang/compiler.hpp"
#include "rylang/res/type_size_from_canonical_type_resolver.hpp"

#include "rylang/manipulators/qmanip.hpp"

void rylang::type_size_from_canonical_type_resolver::process(compiler* c)
{
    type_symbol const& type = m_type;

    assert(!qualified_is_contextual(type));

    auto placement_dp = get_dependency(
        [&]
        {
            return c->lk_type_placement_info_from_canonical_type(type);
        });
    if (!ready())
        return;

    auto placement = placement_dp->get();

    set_value(placement.size);
}
