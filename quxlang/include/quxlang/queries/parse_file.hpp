// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_PARSE_FILE_HEADER_GUARD
#define QUXLANG_QUERIES_PARSE_FILE_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/target_configuration.hpp>

namespace quxlang
{
    struct parse_file_query
    {
        static constexpr auto query_id = "parse_file";
        using input_type = source_file_name;
        using output_type = ast2_file_declaration;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_PARSE_FILE_HEADER_GUARD
