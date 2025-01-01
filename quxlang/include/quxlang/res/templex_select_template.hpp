// Copyright 2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD
#define QUXLANG_RES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD

#include "quxlang/data/type_symbol.hpp"
#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(templex_select_template, instantiation_type, std::optional<temploid_reference>);
}

#endif // RPNX_QUXLANG_TEMPLEX_SELECT_TEMPLATE_HEADER
