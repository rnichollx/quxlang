// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_FUNCTUM_OVERLOADS_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_FUNCTUM_OVERLOADS_SPEC_HEADER_GUARD

#include <quxlang/queries/functum_overloads.hpp>
#include <quxlang/queries/functum_builtin_overloads.hpp>
#include <quxlang/queries/functum_user_overloads.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using functum_overloads_spec = rpnx::querygraph::query_handler_spec< functum_overloads_query, rpnx::typelist< functum_builtin_overloads_query, functum_user_overloads_query > >;

    rpnx::querygraph::coroutine< functum_overloads_spec > functum_overloads_impl(type_symbol input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_FUNCTUM_OVERLOADS_SPEC_HEADER_GUARD
