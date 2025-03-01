//
// Created by Ryan Nicholl on 2024-12-24.
//

#ifndef RPNX_QUXLANG_FUNCTION_HEADER
#define RPNX_QUXLANG_FUNCTION_HEADER

#include "quxlang/data/builtin_functions.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_builtin, temploid_reference, bool);
    QUX_CO_RESOLVER(function_builtin_return_type, temploid_reference, std::optional< type_symbol >);
    QUX_CO_RESOLVER(function_primitive, temploid_reference, std::optional< builtin_function_info >);
    QUX_CO_RESOLVER(function_instanciation, initialization_reference, std::optional< instanciation_reference >);
    QUX_CO_RESOLVER(function_positional_parameter_names, temploid_reference, std::vector< std::string >);
    QUX_CO_RESOLVER(function_param_names, temploid_reference, param_names);
    struct function_ensig_initialize_with_q
    {
        temploid_ensig ensig;
        invotype params;

        RPNX_MEMBER_METADATA(function_ensig_initialize_with_q, ensig, params);
    };

    QUX_CO_RESOLVER(function_ensig_initialize_with, function_ensig_initialize_with_q, std::optional< invotype >);
} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTION_HEADER
