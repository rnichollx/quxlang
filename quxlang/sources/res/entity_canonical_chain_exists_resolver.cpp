// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#include "quxlang/compiler.hpp"

#include "quxlang/manipulators/qmanip.hpp"
#include "quxlang/res/entity_canonical_chain_exists_resolver.hpp"

QUX_CO_RESOLVER_IMPL_FUNC_DEF(exists)
{
    auto typ = co_await QUX_CO_DEP(symbol_type, (input));
    co_return typ != symbol_kind::noexist;
}
