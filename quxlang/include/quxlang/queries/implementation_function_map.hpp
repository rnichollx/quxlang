// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_IMPLEMENTATION_FUNCTION_MAP_HEADER_GUARD
#define QUXLANG_QUERIES_IMPLEMENTATION_FUNCTION_MAP_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <map>

namespace quxlang
{
    struct implementation_function_map_query
    {
        static constexpr auto query_id = "implementation_function_map";
        using input_type = type_symbol;
        using output_type = std::map< interface_slot_key, type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_IMPLEMENTATION_FUNCTION_MAP_HEADER_GUARD
