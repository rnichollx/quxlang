// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD
#define QUXLANG_RES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(functum_select_function, initialization_reference, std::optional< temploid_reference >);
}

#endif // RPNX_QUXLANG_FUNCTUM_SELECT_FUNCTION_HEADER
