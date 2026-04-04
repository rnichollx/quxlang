// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONSTEXPR_U64_HEADER_GUARD
#define QUXLANG_QUERIES_CONSTEXPR_U64_HEADER_GUARD

#include <quxlang/data/constexpr_types.hpp>
#include <cstdint>


namespace quxlang
{
    struct constexpr_u64_query
    {
        static constexpr auto query_id = "constexpr_u64";
        using input_type = constexpr_input;
        using output_type = std::uint64_t;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONSTEXPR_U64_HEADER_GUARD
