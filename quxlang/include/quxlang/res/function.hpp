//
// Created by Ryan Nicholl on 2024-12-24.
//

#ifndef RPNX_QUXLANG_FUNCTION_HEADER
#define RPNX_QUXLANG_FUNCTION_HEADER

#include "quxlang/data/builtin_functions.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_builtin, temploid_reference, std::optional< primitive_function_info >);
    QUX_CO_RESOLVER(function_instanciation, initialization_reference, std::optional< initialization_reference >);
    QUX_CO_RESOLVER(function_positional_parameter_names, temploid_reference, std::vector< std::string >);
} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTION_HEADER
