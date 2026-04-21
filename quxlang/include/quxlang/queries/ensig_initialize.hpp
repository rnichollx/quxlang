// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ENSIG_INITIALIZE_HEADER_GUARD
#define QUXLANG_QUERIES_ENSIG_INITIALIZE_HEADER_GUARD

#include <optional>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct ensig_initialize_query
    {
        static constexpr auto query_id = "ensig_initialize";
        using input_type = ensig_initialization;
        using output_type = std::optional< instatype >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ENSIG_INITIALIZE_HEADER_GUARD
