//
// Created by Ryan Nicholl on 10/27/23.
//

#ifndef QUXLANG_FUNCTION_INSTANCIATION_RESOLVER_HEADER_GUARD
#define QUXLANG_FUNCTION_INSTANCIATION_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/call_parameter_information.hpp"
#include "quxlang/data/type_symbol.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_instanciation, instanciation_reference, std::optional<instanciation_reference>);
} // namespace quxlang

#endif // QUXLANG_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER_GUARD
