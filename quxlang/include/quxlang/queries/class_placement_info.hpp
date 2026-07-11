// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_PLACEMENT_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_PLACEMENT_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/class_placement_info.hpp>


namespace quxlang
{
    /** Computes the object placement required by a class type. */
    struct class_placement_info_query
    {
        static constexpr auto query_id = "class_placement_info";
        using input_type = type_symbol;
        using output_type = class_placement_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_PLACEMENT_INFO_HEADER_GUARD
