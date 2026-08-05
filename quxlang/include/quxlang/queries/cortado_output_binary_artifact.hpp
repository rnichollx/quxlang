// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CORTADO_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
#define QUXLANG_QUERIES_CORTADO_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD

#include <cstddef>
#include <string>
#include <vector>

namespace quxlang
{
    /** Generates the final JAR bytes for one configured Cortado output. */
    struct cortado_output_binary_artifact_query
    {
        static constexpr auto query_id = "cortado_output_binary_artifact";
        using input_type = std::string;
        using output_type = std::vector< std::byte >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CORTADO_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
