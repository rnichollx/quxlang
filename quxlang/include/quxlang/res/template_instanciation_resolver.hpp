//
// Created by Ryan Nicholl on 4/22/24.
//

#ifndef RPNX_QUXLANG_TEMPLATE_INSTANCIATION_HEADER
#define RPNX_QUXLANG_TEMPLATE_INSTANCIATION_HEADER

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(template_instanciation, type_symbol, std::optional<instanciation_reference>);
}

#endif // RPNX_QUXLANG_FUNCTUM_INSTANCIATION_HEADER
