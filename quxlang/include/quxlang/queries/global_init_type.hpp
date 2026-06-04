// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_GLOBAL_INIT_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_GLOBAL_INIT_TYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Classifies the initialization strategy for one global variable symbol.
    struct global_init_type_query
    {
        static constexpr auto query_id = "global_init_type";
        using input_type = type_symbol;
        using output_type = initialization_type;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_GLOBAL_INIT_TYPE_HEADER_GUARD
