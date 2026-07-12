// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_ROUTINE_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_ROUTINE_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>
namespace quxlang
{
    struct constexpr_routine_query
    {
        static constexpr auto query_id = "constexpr_routine";
        using input_type = constexpr_input2;
        using output_type = constexpr_routine_result;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_ROUTINE_HEADER_GUARD
