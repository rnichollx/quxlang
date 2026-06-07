// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_BINARY_INFORMATION_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BINARY_INFORMATION_HEADER_GUARD

#include <quxlang/queries/output_query_output.hpp>

#include <string>

namespace quxlang
{
    /// output_binary_information_query returns metadata for one configured target output.
    struct output_binary_information_query
    {
        static constexpr auto query_id = "output_binary_information";
        using input_type = std::string;
        using output_type = output_query_output;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_BINARY_INFORMATION_HEADER_GUARD
