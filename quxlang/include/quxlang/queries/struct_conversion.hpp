// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_CONVERSION_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_CONVERSION_HEADER_GUARD

#include <quxlang/data/struct_inheritance.hpp>

namespace quxlang
{
    /** Finds an unambiguous static path from one struct type to a base type. */
    struct struct_conversion_query
    {
        static constexpr auto query_id = "struct_conversion";
        using input_type = struct_conversion_input;
        using output_type = struct_conversion_result;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_CONVERSION_HEADER_GUARD
