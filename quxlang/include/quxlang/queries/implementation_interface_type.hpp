// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_IMPLEMENTATION_INTERFACE_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_IMPLEMENTATION_INTERFACE_TYPE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct implementation_interface_type_query
    {
        static constexpr auto query_id = "implementation_interface_type";
        using input_type = type_symbol;
        using output_type = type_symbol;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_IMPLEMENTATION_INTERFACE_TYPE_HEADER_GUARD
