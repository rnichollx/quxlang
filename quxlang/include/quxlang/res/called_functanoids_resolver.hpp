// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include "quxlang/data/vm_procedure.hpp"
#include "rpnx/resolver_utilities.hpp"

#include <set>
#include <quxlang/macros.hpp>

namespace quxlang
{
    class compiler;

    QUX_CO_RESOLVER(called_functanoids, type_symbol, std::set< type_symbol >);

} // namespace quxlang
#endif // CALLED_FUNCTANOIDS_RESOLVER_HEADER_GUARD
