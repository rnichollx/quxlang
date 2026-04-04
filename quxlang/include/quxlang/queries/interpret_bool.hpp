// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INTERPRET_BOOL_HEADER_GUARD
#define QUXLANG_QUERIES_INTERPRET_BOOL_HEADER_GUARD

#include <quxlang/data/interp_value.hpp>


namespace quxlang
{
    struct interpret_bool_query
    {
        static constexpr auto query_id = "interpret_bool";
        using input_type = expr_interp_input;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INTERPRET_BOOL_HEADER_GUARD
