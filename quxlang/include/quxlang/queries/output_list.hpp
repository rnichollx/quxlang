// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_OUTPUT_LIST_HEADER_GUARD
#define QUXLANG_QUERIES_OUTPUT_LIST_HEADER_GUARD

#include <set>
#include <string>
#include <variant>

namespace quxlang
{
    struct output_list_query
    {
        static constexpr auto query_id = "output_list";
        using input_type = std::monostate;
        using output_type = std::set< std::string >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_OUTPUT_LIST_HEADER_GUARD
