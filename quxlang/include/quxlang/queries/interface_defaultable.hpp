// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_INTERFACE_DEFAULTABLE_HEADER_GUARD
#define QUXLANG_QUERIES_INTERFACE_DEFAULTABLE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct interface_defaultable_query
    {
        static constexpr auto query_id = "interface_defaultable";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_INTERFACE_DEFAULTABLE_HEADER_GUARD
