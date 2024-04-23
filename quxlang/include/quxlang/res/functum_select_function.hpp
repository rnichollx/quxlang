//
// Created by Ryan Nicholl on 4/16/24.
//

#ifndef RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER
#define RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(functum_select_function, instanciation_reference, std::optional< selection_reference >);
}

#endif // RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER
