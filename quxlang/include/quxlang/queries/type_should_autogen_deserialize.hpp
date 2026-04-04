// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_SHOULD_AUTOGEN_DESERIALIZE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_SHOULD_AUTOGEN_DESERIALIZE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct type_should_autogen_deserialize_query
    {
        static constexpr auto query_id = "type_should_autogen_deserialize";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_SHOULD_AUTOGEN_DESERIALIZE_HEADER_GUARD
