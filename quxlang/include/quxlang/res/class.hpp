// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_CLASS_LAYOUT_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_CLASS_LAYOUT_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/symbol_id.hpp"

namespace quxlang
{

    QUX_CO_RESOLVER(class_layout, type_symbol, class_layout);
    QUX_CO_RESOLVER(class_builtin, type_symbol, bool);
} // namespace quxlang

#endif // QUXLANG_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
