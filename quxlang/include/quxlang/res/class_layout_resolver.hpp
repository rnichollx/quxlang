//
// Created by Ryan Nicholl on 9/19/23.
//

#ifndef QUXLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
#define QUXLANG_CLASS_LAYOUT_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/class_layout.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/symbol_id.hpp"

namespace quxlang
{

    QUX_CO_RESOLVER(class_layout, type_symbol, class_layout);
} // namespace quxlang

#endif // QUXLANG_CLASS_SIZE_FROM_CANONICAL_CHAIN_RESOLVER_HEADER_GUARD
