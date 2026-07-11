// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/struct_field_declaration.hpp>
#include <vector>


namespace quxlang
{
    /** Lists the declared fields of a struct class before contextual type lookup. */
    struct struct_field_declaration_list_query
    {
        static constexpr auto query_id = "struct_field_declaration_list";
        using input_type = type_symbol;
        using output_type = std::vector< struct_field_declaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_FIELD_DECLARATION_LIST_HEADER_GUARD
