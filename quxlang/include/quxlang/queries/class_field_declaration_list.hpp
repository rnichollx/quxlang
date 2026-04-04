// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CLASS_FIELD_DECLARATION_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_CLASS_FIELD_DECLARATION_LIST_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/data/class_field_declaration.hpp>
#include <vector>


namespace quxlang
{
    struct class_field_declaration_list_query
    {
        static constexpr auto query_id = "class_field_declaration_list";
        using input_type = type_symbol;
        using output_type = std::vector< class_field_declaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CLASS_FIELD_DECLARATION_LIST_HEADER_GUARD
