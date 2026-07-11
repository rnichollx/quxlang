// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_LAYOUT_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_LAYOUT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/struct_layout.hpp>


namespace quxlang
{
    /** Computes the field layout of a struct class. */
    struct struct_layout_query
    {
        static constexpr auto query_id = "struct_layout";
        using input_type = type_symbol;
        using output_type = struct_layout;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_LAYOUT_HEADER_GUARD
