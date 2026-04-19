// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_MODULE_OPTIONS_MAP_HEADER_GUARD
#define QUXLANG_QUERIES_MODULE_OPTIONS_MAP_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <map>
#include <string>

namespace quxlang
{
    struct module_options_map_query
    {
        static constexpr auto query_id = "module_options_map";
        using input_type = std::monostate;
        using output_type = std::map< type_symbol, std::string >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_MODULE_OPTIONS_MAP_HEADER_GUARD
