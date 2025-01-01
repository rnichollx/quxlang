// Copyright 2023-2024 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_RES_LIST_USER_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD
#define QUXLANG_RES_LIST_USER_FUNCTUM_OVERLOADS_RESOLVER_HEADER_GUARD

#include "quxlang/compiler_fwd.hpp"
#include "quxlang/data/type_symbol.hpp"
#include "rpnx/resolver_utilities.hpp"
#include "quxlang/ast2/ast2_entity.hpp"

namespace quxlang
{
    // Lists function overloads for a functum, they are returned in the function-order.
    QUX_CO_RESOLVER(list_user_functum_overload_declarations, type_symbol, std::vector< ast2_function_declaration >);
    QUX_CO_RESOLVER(list_user_functum_formal_paratypes, type_symbol, std::vector< paratype >);
    QUX_CO_RESOLVER(list_functum_overloads, type_symbol, std::set< temploid_formal_paratype >);
    QUX_CO_RESOLVER(list_user_functum_overloads, type_symbol, std::vector< temploid_formal_paratype >);
    QUX_CO_RESOLVER(function_declaration, selection_reference, std::optional<ast2_function_declaration>);
} // namespace quxlang

#endif // FUNCTUM_BUILTIN_OVERLOADS_RESOLVER_HEADER_GUARD
