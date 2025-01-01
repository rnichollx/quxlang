//
// Created by Ryan Nicholl on 2024-12-21.
//

#ifndef RPNX_QUXLANG_CONSTRUCTOR_HEADER
#define RPNX_QUXLANG_CONSTRUCTOR_HEADER


#include "quxlang/data/expression.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(nontrivial_default_dtor, type_symbol, std::optional<initialization_reference>);
}

#endif // RPNX_QUXLANG_CONSTRUCTOR_HEADER
