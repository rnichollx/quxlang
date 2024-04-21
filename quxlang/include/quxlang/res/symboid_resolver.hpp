//
// Created by Ryan Nicholl on 4/20/24.
//

#ifndef RPNX_QUXLANG_SYMBOID_RESOLVER_HEADER
#define RPNX_QUXLANG_SYMBOID_RESOLVER_HEADER

#include <quxlang/res/resolver.hpp>

#include <quxlang/ast2/ast2_entity.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(symboid, type_symbol, ast2_symboid);
}


#endif // RPNX_QUXLANG_SYMBOID_RESOLVER_HEADER
