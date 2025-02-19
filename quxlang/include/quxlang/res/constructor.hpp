//
// Created by Ryan Nicholl on 2024-12-21.
//

#ifndef RPNX_QUXLANG_CONSTRUCTOR_HEADER
#define RPNX_QUXLANG_CONSTRUCTOR_HEADER


#include "quxlang/data/expression.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(nontrivial_builtin_default_dtor, type_symbol, bool);
    QUX_CO_RESOLVER(nontrivial_builtin_default_ctor, type_symbol, bool);

    QUX_CO_RESOLVER(user_default_dtor, type_symbol, std::optional<instanciation_reference>);
    QUX_CO_RESOLVER(user_default_ctor, type_symbol, std::optional<instanciation_reference>);

    QUX_CO_RESOLVER(trivially_destructible, type_symbol, bool);
    QUX_CO_RESOLVER(trivially_constructible, type_symbol, bool);

    QUX_CO_RESOLVER(default_dtor, type_symbol, std::optional<instanciation_reference>);
    QUX_CO_RESOLVER(default_ctor, type_symbol, std::optional<instanciation_reference>);
}

#endif // RPNX_QUXLANG_CONSTRUCTOR_HEADER
