// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_TAGS_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_TAGS_HEADER_GUARD

#include <quxlang/data/struct_tags_types.hpp>
#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    /** Returns the keyword tags attached to a struct declaration. */
    struct struct_tags_query
    {
        static constexpr auto query_id = "struct_tags";
        using input_type = type_symbol;
        using output_type = struct_tags_result_type;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_TAGS_HEADER_GUARD
