// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_ROUTINE_V3_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_ROUTINE_V3_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>

namespace quxlang
{
    struct constexpr_routine_v3_query
    {
        /// Querygraph identifier for constexpr v3 VMIR generation.
        static constexpr auto query_id = "constexpr_routine_v3";
        /// Versioned constexpr generation request.
        using input_type = constexpr_input_v3;
        /// Generated VMIR routine plus primary AUTO deduction metadata.
        using output_type = constexpr_routine_v3_result;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_ROUTINE_V3_HEADER_GUARD
