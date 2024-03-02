//
// Created by Ryan Nicholl on 11/25/23.
//

#ifndef FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD
#define FUNCTUM_USER_OVERLOADS_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "rylang/compiler_fwd.hpp"
#include "rylang/data/qualified_symbol_reference.hpp"

namespace rylang
{
    QUX_CO_RESOLVER(list_user_functum_overloads, type_symbol, std::set< call_parameter_information >)
} // namespace rylang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
