// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SOURCE_FILE_INDEX_HEADER_GUARD
#define QUXLANG_QUERIES_SOURCE_FILE_INDEX_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <variant>

namespace quxlang
{
    struct source_file_index_query
    {
        static constexpr auto query_id = "source_file_index";
        using input_type = std::monostate;
        using output_type = source_file_index;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SOURCE_FILE_INDEX_HEADER_GUARD
