// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_CORTADO_INPUT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_CORTADO_INPUT_HEADER_GUARD

#include <quxlang/cortado-backend-types.hpp>

#include <string>

namespace quxlang
{
    /** Aggregates the layout-independent runtime closure for one Cortado output. */
    struct output_cortado_input_query
    {
        static constexpr auto query_id = "output_cortado_input";
        using input_type = std::string;
        using output_type = cortado_backend::cortado_compilable_unit;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_CORTADO_INPUT_HEADER_GUARD
