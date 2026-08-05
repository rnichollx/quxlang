// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_CORTADO_BACKEND_OPTIONS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_CORTADO_BACKEND_OPTIONS_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <string>

namespace quxlang
{
    /** Returns effective Cortado backend options for one configured output. */
    struct output_cortado_backend_options_query
    {
        static constexpr auto query_id = "output_cortado_backend_options";
        using input_type = std::string;
        using output_type = backend_cortado_options;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_CORTADO_BACKEND_OPTIONS_HEADER_GUARD
