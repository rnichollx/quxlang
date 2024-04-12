//
// Created by Ryan Nicholl on 3/31/24.
//

#ifndef RPNX_QUXLANG_INTERPRET_BOOL_RESOLVER_HEADER
#define RPNX_QUXLANG_INTERPRET_BOOL_RESOLVER_HEADER


#include "quxlang/data/interp_value.hpp"

#include "quxlang/macros.hpp"

namespace quxlang
{
   QUX_CO_RESOLVER(interpret_bool, expr_interp_input, bool);
}
#endif // RPNX_QUXLANG_INTERPRET_BOOL_RESOLVER_HEADER
