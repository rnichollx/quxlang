// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_STEPPINGS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_STEPPINGS_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>
#include <vector>

namespace quxlang
{
    /** Returns the ordered CPU stepping configurations used by the configured output. */
    struct output_steppings_query
    {
        static constexpr auto query_id = "output_steppings";
        using input_type = std::string;
        using output_type = std::vector< cpu_stepping_configuration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_STEPPINGS_HEADER_GUARD
