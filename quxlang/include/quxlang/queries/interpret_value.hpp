// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INTERPRET_VALUE_HEADER_GUARD
#define QUXLANG_QUERIES_INTERPRET_VALUE_HEADER_GUARD

#include <quxlang/data/interp_value.hpp>


namespace quxlang
{
    struct interpret_value_query
    {
        static constexpr auto query_id = "interpret_value";
        using input_type = expr_interp_input;
        using output_type = interp_value;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INTERPRET_VALUE_HEADER_GUARD
