// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_BINARIES_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_BINARIES_HEADER_GUARD

#include <quxlang/queries/output_query_output.hpp>

#include <map>
#include <variant>

namespace quxlang
{
    struct output_binaries_query
    {
        static constexpr auto query_id = "output_binaries";
        using input_type = std::monostate;
        using output_type = std::map< std::string, output_query_output >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_BINARIES_HEADER_GUARD
