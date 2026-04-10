// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_LIST_USER_FUNCTUM_FORMAL_PARATYPES_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_LIST_USER_FUNCTUM_FORMAL_PARATYPES_SPEC_HEADER_GUARD

#include <quxlang/queries/list_user_functum_formal_paratypes.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>
#include <quxlang/queries/lookup.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using list_user_functum_formal_paratypes_spec = rpnx::querygraph::query_handler_spec< list_user_functum_formal_paratypes_query, rpnx::typelist< functum_list_user_overload_declarations_query, lookup_query > >;

    rpnx::querygraph::coroutine< list_user_functum_formal_paratypes_spec > list_user_functum_formal_paratypes_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_LIST_USER_FUNCTUM_FORMAL_PARATYPES_SPEC_HEADER_GUARD
