// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_BOOL_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_BOOL_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_bool.hpp>
#include <quxlang/queries/constexpr_eval.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_bool_spec = rpnx::query_handler_spec< constexpr_bool_query, rpnx::typelist< constexpr_eval_query > >;

    rpnx::querygraph::coroutine< constexpr_bool_spec > constexpr_bool_impl(constexpr_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_BOOL_SPEC_HEADER_GUARD
