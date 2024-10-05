//
// Created by Ryan Nicholl on 11/18/23.
//

#ifndef QUXLANG_RES_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(list_functum_overloads, type_symbol, std::set< function_overload >);
} // namespace quxlang


#endif // QUXLANG_LIST_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
