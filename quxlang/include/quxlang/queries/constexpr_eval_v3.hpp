// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_EVAL_V3_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_EVAL_V3_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    struct constexpr_eval_v3_query
    {
        /// Querygraph identifier for versioned constexpr evaluation.
        static constexpr auto query_id = "constexpr_eval_v3";
        /// Expression, context, static state, and expected result policy.
        using input_type = constexpr_input_v3;
        /// All emitted constexpr result IDs plus optional AUTO deduction metadata.
        using output_type = constexpr_result_v3;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_EVAL_V3_HEADER_GUARD
