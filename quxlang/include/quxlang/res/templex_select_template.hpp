//
// Created by Ryan Nicholl on 4/22/24.
//

#ifndef QUXLANG_RES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD
#define QUXLANG_RES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(templex_select_template, instanciation_reference, std::optional<selection_reference>);
}

#endif // RPNX_QUXLANG_TEMPLEX_SELECT_TEMPLATE_HEADER
