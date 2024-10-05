// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_SYMBOID_SUBDECLAROIDS_HEADER_GUARD
#define QUXLANG_RES_SYMBOID_SUBDECLAROIDS_HEADER_GUARD

#include "quxlang/ast2/ast2_entity.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(symboid_subdeclaroids, type_symbol, std::vector<subdeclaroid>);
}

#endif // RPNX_QUXLANG_SYMBOID_SUBDECLAROIDS_HEADER
