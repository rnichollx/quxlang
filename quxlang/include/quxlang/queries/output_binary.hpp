// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_BINARY_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BINARY_HEADER_GUARD

#include <quxlang/queries/output_query_output.hpp>

#include <string>

namespace quxlang
{
    struct output_binary_query
    {
        static constexpr auto query_id = "output_binary";
        using input_type = std::string;
        using output_type = output_query_output;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_BINARY_HEADER_GUARD
