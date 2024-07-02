//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER
#define RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER

#include "quxlang/data/expression.hpp"
#include "quxlang/data/interp_value.hpp"
#include "quxlang/macros.hpp"
#include "quxlang/vmir2/vmir2.hpp"

namespace quxlang
{


    QUX_CO_RESOLVER(interpret_value, expr_interp_input, interp_value);
} // namespace quxlang

#endif // RPNX_QUXLANG_INTERPRET_VALUE_RESOLVER_HEADER
