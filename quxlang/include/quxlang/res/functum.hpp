// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD
#define QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(functum_select_function, initialization_reference, std::optional< temploid_reference >);
    QUX_CO_RESOLVER(functum_exists_and_is_callable_with, initialization_reference, bool);
    QUX_CO_RESOLVER(functum_initialize, initialization_reference, std::optional< instanciation_reference >);
} // namespace quxlang

#endif // RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER
