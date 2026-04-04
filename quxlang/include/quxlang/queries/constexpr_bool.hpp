// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_BOOL_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_BOOL_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>


namespace quxlang
{
    struct constexpr_bool_query
    {
        static constexpr auto query_id = "constexpr_bool";
        using input_type = constexpr_input;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_BOOL_HEADER_GUARD
