// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SYMBOL_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_SYMBOL_TYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/symbol_type.hpp>


namespace quxlang
{
    struct symbol_type_query
    {
        static constexpr auto query_id = "symbol_type";
        using input_type = type_symbol;
        using output_type = symbol_kind;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SYMBOL_TYPE_HEADER_GUARD
