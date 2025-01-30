// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"


void quxlang::class_should_autogen_default_constructor_resolver::process(compiler* c)
{

    auto callee = submember{m_cls, "CONSTRUCTOR"};
    auto exists_dp = get_dependency(
        [&]
        {
            return c->lk_exists(callee);
        });
    if (!ready())
    {
        return;
    }
    bool exists = exists_dp->get();

    set_value(!exists);
}
