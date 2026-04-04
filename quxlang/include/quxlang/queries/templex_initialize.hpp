// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLEX_INITIALIZE_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLEX_INITIALIZE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <optional>


namespace quxlang
{
    struct templex_initialize_query
    {
        static constexpr auto query_id = "templex_initialize";
        using input_type = initialization_reference;
        using output_type = std::optional< instanciation_reference >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLEX_INITIALIZE_HEADER_GUARD
