// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_INTERPRET_VALUE_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_INTERPRET_VALUE_SPEC_HEADER_GUARD

#include <quxlang/queries/interpret_value.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using interpret_value_spec = rpnx::query_handler_spec< interpret_value_query, rpnx::typelist<  > >;

    rpnx::querygraph::coroutine< interpret_value_spec > interpret_value_impl(expr_interp_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_INTERPRET_VALUE_SPEC_HEADER_GUARD
