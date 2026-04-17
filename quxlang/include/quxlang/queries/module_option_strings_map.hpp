// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_OPTION_STRINGS_MAP_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_OPTION_STRINGS_MAP_HEADER_GUARD

#include <map>
#include <string>
#include <variant>

namespace quxlang
{
    struct module_option_strings_map_query
    {
        static constexpr auto query_id = "module_option_strings_map";
        using input_type = std::monostate;
        using output_type = std::map< std::string, std::map< std::string, std::string > >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_OPTION_STRINGS_MAP_HEADER_GUARD
