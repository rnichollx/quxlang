// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_IS_IMPLICITLY_DATATYPE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_IS_IMPLICITLY_DATATYPE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct type_is_implicitly_datatype_query
    {
        static constexpr auto query_id = "type_is_implicitly_datatype";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_IS_IMPLICITLY_DATATYPE_HEADER_GUARD
