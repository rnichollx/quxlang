//
// Created by Ryan Nicholl on 10/20/23.
//

#ifndef QUXLANG_LOOKUP_RESOLVER_HEADER_GUARD
#define QUXLANG_LOOKUP_RESOLVER_HEADER_GUARD

// input: contextual_type_reference
// output: canonical_type_reference

#include "quxlang/data/canonical_type_reference.hpp"
#include "quxlang/data/contextual_type_reference.hpp"

#include "quxlang/res/resolver.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"

namespace quxlang
{

    QUX_CO_RESOLVER(lookup, contextual_type_reference, std::optional<type_symbol>);

} // namespace quxlang

#endif // QUXLANG_CANONICAL_SYMBOL_FROM_CONTEXTUAL_SYMBOL_RESOLVER_HEADER_GUARD
