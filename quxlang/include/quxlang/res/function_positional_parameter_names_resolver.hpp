//
// Created by Ryan Nicholl on 4/18/24.
//

#ifndef RPNX_QUXLANG_FUNCTION_POSITIONAL_PARAMETER_NAMES_RESOLVER_HEADER
#define RPNX_QUXLANG_FUNCTION_POSITIONAL_PARAMETER_NAMES_RESOLVER_HEADER

#include <quxlang/data/type_symbol.hpp>

#include <quxlang/macros.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_positional_parameter_names, selection_reference, std::vector< std::string >);
}

#endif // RPNX_QUXLANG_FUNCTION_POSITIONAL_PARAMETER_NAMES_RESOLVER_HEADER
