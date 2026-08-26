// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_MEMBER_LOOKUP_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_MEMBER_LOOKUP_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Resolves one member name through a normalized struct hierarchy. */
    struct struct_member_lookup_query
    {
        static constexpr auto query_id = "struct_member_lookup";
        using input_type = struct_member_lookup_input;
        using output_type = struct_member_lookup_result;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_MEMBER_LOOKUP_HEADER_GUARD
