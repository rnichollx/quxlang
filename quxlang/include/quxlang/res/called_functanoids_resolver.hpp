//
// Created by Ryan Nicholl on 11/30/23.
//

#ifndef CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
#define CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/vm_procedure.hpp"
#include "rpnx/resolver_utilities.hpp"

#include <set>

namespace quxlang
{
    class compiler;

    QUX_CO_RESOLVER(called_functanoids, type_symbol, std::set< type_symbol >);

} // namespace quxlang
#endif // CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
