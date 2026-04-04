// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_USER_SWAP_EXISTS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_USER_SWAP_EXISTS_SPEC_HEADER_GUARD

#include <quxlang/queries/user_swap_exists.hpp>
#include <quxlang/queries/functum_user_overloads.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using user_swap_exists_spec = rpnx::query_handler_spec< user_swap_exists_query, rpnx::typelist< functum_user_overloads_query > >;

    rpnx::querygraph::coroutine< user_swap_exists_spec > user_swap_exists_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_USER_SWAP_EXISTS_SPEC_HEADER_GUARD
