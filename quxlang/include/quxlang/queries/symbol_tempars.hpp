// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SYMBOL_TEMPARS_HEADER_GUARD
#define QUXLANG_QUERIES_SYMBOL_TEMPARS_HEADER_GUARD

#include <quxlang/data/tempar_types.hpp>
#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct symbol_tempars_query
    {
        static constexpr auto query_id = "symbol_tempars";
        using input_type = type_symbol;
        using output_type = tempar_name_set;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SYMBOL_TEMPARS_HEADER_GUARD
