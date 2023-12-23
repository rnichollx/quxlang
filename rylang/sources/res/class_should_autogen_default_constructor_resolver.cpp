//
// Created by Ryan Nicholl on 11/14/23.
//
#include "rylang/compiler.hpp"
#include "rylang/res/class_should_autogen_default_constructor_resolver.hpp"


void rylang::class_should_autogen_default_constructor_resolver::process(compiler* c)
{

    auto callee = subdotentity_reference{m_cls, "CONSTRUCTOR"};
    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_entity_canonical_chain_exists(callee);
        });
    if (!ready())
    {
        return;
    }
    bool exists = exists_dp->get();

    set_value(!exists);
}
