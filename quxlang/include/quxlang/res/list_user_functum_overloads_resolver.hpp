//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD
#define FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(list_user_functum_overloads, type_symbol, std::set<ast2_function_header>)
} // namespace quxlang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
