//
// Created by Ryan Nicholl on 4/22/24.
//

#ifndef RPNX_QUXLANG_SYMBOL_TYPE_HEADER
#define RPNX_QUXLANG_SYMBOL_TYPE_HEADER

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>
#include <rpnx/metadata.hpp>

RPNX_ENUM(quxlang, symbol_kind, std::int64_t, noexist, module, builtin_class, user_class, functum, user_function, builtin_function, funtanoid, variable, templex, template_, namespace_, argument)

namespace quxlang
{
    QUX_CO_RESOLVER(symbol_type, type_symbol, symbol_kind);

}

#endif // RPNX_QUXLANG_SYMBOL_TYPE_HEADER
