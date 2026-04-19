// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_EXISTS_HEADER_GUARD
#define QUXLANG_QUERIES_EXISTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    struct exists_query
    {
        static constexpr auto query_id = "exists";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_EXISTS_HEADER_GUARD
