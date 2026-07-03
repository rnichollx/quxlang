// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_NUMERIC_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_NUMERIC_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_eval_numeric.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_eval_numeric_spec
    {
        using query = constexpr_eval_numeric_query;
        using dependencies = rpnx::typelist< constexpr_eval_v3_query >;
    };

    rpnx::querygraph::coroutine< constexpr_eval_numeric_spec > constexpr_eval_numeric_impl(constexpr_input_v3 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_NUMERIC_SPEC_HEADER_GUARD
