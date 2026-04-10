// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INTERPRET_BOOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INTERPRET_BOOL_SPEC_HEADER_GUARD

#include <quxlang/queries/interpret_bool.hpp>
#include <quxlang/queries/interpret_value.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using interpret_bool_spec = rpnx::querygraph::query_handler_spec< interpret_bool_query, rpnx::typelist< interpret_value_query > >;

    rpnx::querygraph::coroutine< interpret_bool_spec > interpret_bool_impl(expr_interp_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INTERPRET_BOOL_SPEC_HEADER_GUARD
