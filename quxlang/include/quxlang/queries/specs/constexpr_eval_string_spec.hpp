// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_STRING_SPEC_HEADER_GUARD
#define QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_STRING_SPEC_HEADER_GUARD

#include <quxlang/queries/constexpr_eval_string.hpp>
#include <quxlang/queries/constexpr_eval_v3.hpp>

#include <new>
#include <rpnx/querygraph/querygraph.hpp>

namespace quxlang
{
    struct constexpr_eval_string_spec
    {
        using query = constexpr_eval_string_query;
        using dependencies = rpnx::typelist< constexpr_eval_v3_query >;
    };

    /// Evaluates the input expression with constexpr v3 and returns the primary result as string bytes.
    rpnx::querygraph::coroutine< constexpr_eval_string_spec > constexpr_eval_string_impl(constexpr_input_v3 input);
} // namespace quxlang

#endif // QUXLANG_QUERIES_SPECS_CONSTEXPR_EVAL_STRING_SPEC_HEADER_GUARD
