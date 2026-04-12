// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_ROUTINE_ANTESTATAL_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_ROUTINE_ANTESTATAL_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>
#include <quxlang/vmir2/vmir2.hpp>

namespace quxlang
{
    struct constexpr_routine_antestatal_query
    {
        static constexpr auto query_id = "constexpr_routine_antestatal";
        using input_type = constexpr_input2;
        using output_type = vmir2::functanoid_routine3;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_ROUTINE_ANTESTATAL_HEADER_GUARD
