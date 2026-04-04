// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_SOURCE_NAME_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_SOURCE_NAME_HEADER_GUARD

#include <string>

namespace quxlang
{
    struct module_source_name_query
    {
        static constexpr auto query_id = "module_source_name";
        using input_type = std::string;
        using output_type = std::string;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_SOURCE_NAME_HEADER_GUARD
