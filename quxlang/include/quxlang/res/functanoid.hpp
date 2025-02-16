//
// Created by Ryan Nicholl on 2024-12-24.
//

#ifndef RPNX_QUXLANG_FUNCTANOID_HEADER
#define RPNX_QUXLANG_FUNCTANOID_HEADER

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"
#include <rpnx/resolver_utilities.hpp>
#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/temploid_instanciation_parameter_set.hpp>

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(functanoid_parameter_map, instanciation_reference, temploid_instanciation_parameter_set);
    QUX_CO_RESOLVER(functanoid_return_type, instanciation_reference, type_symbol);
    QUX_CO_RESOLVER(functanoid_sigtype, instanciation_reference, sigtype);

} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTANOID_HEADER
