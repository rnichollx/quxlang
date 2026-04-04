// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_USER_DEFAULT_DTOR_EXISTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_USER_DEFAULT_DTOR_EXISTS_SPEC_HEADER_GUARD

#include <quxlang/queries/user_default_dtor_exists.hpp>
#include <quxlang/queries/function_ensig_init_with.hpp>
#include <quxlang/queries/functum_overloads.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using user_default_dtor_exists_spec = rpnx::query_handler_spec< user_default_dtor_exists_query, rpnx::typelist< function_ensig_init_with_query, functum_overloads_query > >;

    rpnx::querygraph::coroutine< user_default_dtor_exists_spec > user_default_dtor_exists_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_USER_DEFAULT_DTOR_EXISTS_SPEC_HEADER_GUARD
