//
// Created by Ryan Nicholl on 10/24/23.
//

#ifndef QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
#define QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/canonical_lookup_chain.hpp"
#include "quxlang/data/lookup_chain.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    class [[deprecated("Use symbol_type instead")]] entity_canonical_chain_exists ;
    QUX_CO_RESOLVER(entity_canonical_chain_exists, type_symbol, bool);
}

#endif // QUXLANG_ENTITY_CANONICAL_CHAIN_EXISTS_RESOLVER_HEADER_GUARD
