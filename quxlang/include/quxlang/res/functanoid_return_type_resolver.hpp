//
// Created by Ryan Nicholl on 11/23/23.
//

#ifndef QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(functanoid_return_type, type_symbol, type_symbol);
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD
