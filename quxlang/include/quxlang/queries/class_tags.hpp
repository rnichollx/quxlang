// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_TAGS_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_TAGS_HEADER_GUARD

#include <quxlang/data/class_tags_types.hpp>
#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct class_tags_query
    {
        static constexpr auto query_id = "class_tags";
        using input_type = type_symbol;
        using output_type = class_tags_result_type;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_TAGS_HEADER_GUARD
