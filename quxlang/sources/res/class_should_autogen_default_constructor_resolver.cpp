// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com
#include "quxlang/compiler.hpp"
#include "quxlang/res/class_should_autogen_default_constructor_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(class_should_autogen_default_constructor)
{
    co_return co_await QUX_CO_DEP(nontrivial_default_ctor, (input));
}
