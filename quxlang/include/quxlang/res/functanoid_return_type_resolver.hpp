// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_FUNCTANOID_RETURN_TYPE_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_FUNCTANOID_RETURN_TYPE_RESOLVER_HEADER_GUARD

#include "rpnx/resolver_utilities.hpp"
#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"

#include "quxlang/macros.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(functanoid_return_type, instanciation_reference, type_symbol);
} // namespace quxlang

#endif // QUXLANG_FUNCTION_RETURN_TYPE_RESOLVER_HEADER_GUARD
