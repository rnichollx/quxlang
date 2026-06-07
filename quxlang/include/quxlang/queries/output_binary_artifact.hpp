// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <cstddef>
#include <rpnx/macros.hpp>
#include <string>
#include <vector>

namespace quxlang
{
    /// output_binary_artifact is the final byte artifact for one qxc output.
    struct output_binary_artifact
    {
        std::string output_name;
        output_kind type = output_kind::executable;
        std::vector< std::byte > bytes;

        RPNX_MEMBER_METADATA(output_binary_artifact, output_name, type, bytes);
    };

    /// output_binary_artifact_query returns the final artifact for one configured output.
    struct output_binary_artifact_query
    {
        static constexpr auto query_id = "output_binary_artifact";
        using input_type = std::string;
        using output_type = output_binary_artifact;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACT_HEADER_GUARD
