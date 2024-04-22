//
// Created by Ryan Nicholl on 4/21/24.
//

#ifndef RPNX_QUXLANG_SYMBOID_SUBDECLAROIDS_HEADER
#define RPNX_QUXLANG_SYMBOID_SUBDECLAROIDS_HEADER

#include "quxlang/ast2/ast2_entity.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(symboid_subdeclaroids, type_symbol, std::vector<subdeclaroid>);
}

#endif // RPNX_QUXLANG_SYMBOID_SUBDECLAROIDS_HEADER
