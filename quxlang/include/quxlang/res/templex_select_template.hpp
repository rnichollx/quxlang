//
// Created by Ryan Nicholl on 4/22/24.
//

#ifndef RPNX_QUXLANG_TEMPLEX_SELECT_TEMPLATE_HEADER
#define RPNX_QUXLANG_TEMPLEX_SELECT_TEMPLATE_HEADER

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(templex_select_template, instanciation_reference, std::optional<selection_reference>);
}

#endif // RPNX_QUXLANG_TEMPLEX_SELECT_TEMPLATE_HEADER
