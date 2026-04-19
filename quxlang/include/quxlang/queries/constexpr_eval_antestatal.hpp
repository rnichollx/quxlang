// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_EVAL_ANTESTATAL_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_EVAL_ANTESTATAL_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    struct constexpr_eval_antestatal_query
    {
        static constexpr auto query_id = "constexpr_eval_antestatal";
        using input_type = constexpr_input2;
        using output_type = antestatal_value;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_EVAL_ANTESTATAL_HEADER_GUARD
