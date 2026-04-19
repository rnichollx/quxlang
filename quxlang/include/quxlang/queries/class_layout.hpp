// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_LAYOUT_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_LAYOUT_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_layout.hpp>


namespace quxlang
{
    struct class_layout_query
    {
        static constexpr auto query_id = "class_layout";
        using input_type = type_symbol;
        using output_type = class_layout;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_LAYOUT_HEADER_GUARD
