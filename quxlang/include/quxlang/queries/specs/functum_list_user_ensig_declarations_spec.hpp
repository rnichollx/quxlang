// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_list_user_ensig_declarations.hpp>
#include <quxlang/queries/functum_list_user_overload_declarations.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct functum_list_user_ensig_declarations_spec
    {
        using query = functum_list_user_ensig_declarations_query;
        using dependencies = rpnx::typelist< functum_list_user_overload_declarations_query >;
    };

    rpnx::querygraph::coroutine< functum_list_user_ensig_declarations_spec > functum_list_user_ensig_declarations_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_LIST_USER_ENSIG_DECLARATIONS_SPEC_HEADER_GUARD
