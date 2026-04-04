// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LOOKUP_HEADER_GUARD
#define QUXLANG_QUERIES_LOOKUP_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/contextual_type_reference.hpp>
#include <optional>


namespace quxlang
{
    struct lookup_query
    {
        static constexpr auto query_id = "lookup";
        using input_type = contextual_type_reference;
        using output_type = std::optional<type_symbol>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LOOKUP_HEADER_GUARD
