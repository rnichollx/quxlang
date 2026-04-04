// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_U64_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_U64_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_u64.hpp>
#include <quxlang/queries/constexpr_eval.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    using constexpr_u64_spec = rpnx::query_handler_spec< constexpr_u64_query, rpnx::typelist< constexpr_eval_query > >;

    rpnx::querygraph::coroutine< constexpr_u64_spec > constexpr_u64_impl(constexpr_input input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_U64_SPEC_HEADER_GUARD
