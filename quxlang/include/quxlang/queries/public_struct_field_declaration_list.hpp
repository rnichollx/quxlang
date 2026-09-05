// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD

#include <quxlang/data/struct_field_declaration.hpp>
#include <vector>

namespace quxlang
{
    /** Lists direct public fields of a resolved named struct in declaration order. */
    struct public_struct_field_declaration_list_query
    {
        static constexpr auto query_id = "public_struct_field_declaration_list";
        using input_type = type_symbol;
        using output_type = std::vector< struct_field_declaration >;
    };
}

#endif // QUXLANG_QUERIES_PUBLIC_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
