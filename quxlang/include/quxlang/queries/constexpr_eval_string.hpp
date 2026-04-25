// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_EVAL_STRING_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_EVAL_STRING_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    struct constexpr_eval_string_query
    {
        static constexpr auto query_id = "constexpr_eval_string";
        using input_type = constexpr_input_v3;
        using output_type = constexpr_string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_EVAL_STRING_HEADER_GUARD
