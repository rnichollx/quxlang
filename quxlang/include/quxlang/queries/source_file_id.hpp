// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SOURCE_FILE_ID_HEADER_GUARD
#define QUXLANG_QUERIES_SOURCE_FILE_ID_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <cstdint>

namespace quxlang
{
    struct source_file_id_query
    {
        static constexpr auto query_id = "source_file_id";
        using input_type = source_file_name;
        using output_type = std::uint64_t;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SOURCE_FILE_ID_HEADER_GUARD
