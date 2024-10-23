// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTION_INSTANCIATION_HEADER_GUARD
#define QUXLANG_RES_FUNCTION_INSTANCIATION_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

#include <quxlang/res/resolver.hpp>

namespace quxlang
{
    QUX_CO_RESOLVER(function_instanciation, instantiation_type, std::optional<instantiation_type>);
} // namespace quxlang

#endif // QUXLANG_FUNCTION_OVERLOAD_SELECTION_RESOLVER_HEADER_GUARD
