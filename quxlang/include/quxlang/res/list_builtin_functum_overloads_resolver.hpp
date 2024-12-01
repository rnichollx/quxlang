// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_LIST_BUILTIN_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_LIST_BUILTIN_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/builtin_functions.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "quxlang/macros.hpp"
#include "rpnx/resolver_utilities.hpp"

namespace quxlang
{
    QUX_CO_RESOLVER(list_builtin_functum_overloads, type_symbol, std::set<primitive_function_info>);
    QUX_CO_RESOLVER(list_builtin_constructors, type_symbol, std::set<primitive_function_info>);
} // namespace quxlang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
