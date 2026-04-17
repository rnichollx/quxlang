// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SOURCE_FILE_NAME_HEADER_GUARD
#define QUXLANG_QUERIES_SOURCE_FILE_NAME_HEADER_GUARD

#include <quxlang/data/target_configuration.hpp>

#include <cstdint>

namespace quxlang
{
    struct source_file_name_query
    {
        static constexpr auto query_id = "source_file_name";
        using input_type = std::uint64_t;
        using output_type = source_file_name;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SOURCE_FILE_NAME_HEADER_GUARD
