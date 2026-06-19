// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACTS_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACTS_HEADER_GUARD

#include <quxlang/queries/output_binary_artifact.hpp>

#include <map>
#include <variant>

namespace quxlang
{
    /// output_binary_artifacts_query returns final artifacts for all configured outputs.
    struct output_binary_artifacts_query
    {
        static constexpr auto query_id = "output_binary_artifacts";
        using input_type = std::monostate;
        using output_type = std::map< std::string, std::vector<std::byte> >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_BINARY_ARTIFACTS_HEADER_GUARD
