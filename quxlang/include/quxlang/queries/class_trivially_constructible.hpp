// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_TRIVIALLY_CONSTRUCTIBLE_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_TRIVIALLY_CONSTRUCTIBLE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct class_trivially_constructible_query
    {
        static constexpr auto query_id = "class_trivially_constructible";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_TRIVIALLY_CONSTRUCTIBLE_HEADER_GUARD
