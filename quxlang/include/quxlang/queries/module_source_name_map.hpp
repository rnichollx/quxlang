// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_SOURCE_NAME_MAP_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_SOURCE_NAME_MAP_HEADER_GUARD

#include <map>
#include <string>
#include <variant>

namespace quxlang
{
    struct module_source_name_map_query
    {
        static constexpr auto query_id = "module_source_name_map";
        using input_type = std::monostate;
        using output_type = std::map< std::string, std::string >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_SOURCE_NAME_MAP_HEADER_GUARD
