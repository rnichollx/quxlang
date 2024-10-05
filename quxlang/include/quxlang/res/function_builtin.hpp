//
// Created by Ryan Nicholl on 4/26/24.
//

#ifndef QUXLANG_RES_FUNCTION_BUILTIN_HEADER_GUARD
#define QUXLANG_RES_FUNCTION_BUILTIN_HEADER_GUARD

#include "quxlang/data/builtin_functions.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_builtin, selection_reference, std::optional<primitive_function_info>);
}

#endif // RPNX_QUXLANG_FUNCTION_BUILTIN_HEADER
