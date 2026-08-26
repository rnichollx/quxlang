// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_INHERITANCE_INFO_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_INHERITANCE_INFO_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Returns the canonical inheritance graph and validates transitive hierarchy rules. */
    struct struct_inheritance_info_query
    {
        static constexpr auto query_id = "struct_inheritance_info";
        using input_type = type_symbol;
        using output_type = struct_inheritance_info;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_INHERITANCE_INFO_HEADER_GUARD
