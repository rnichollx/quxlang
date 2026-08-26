// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_STRUCT_DIRECT_BASES_HEADER_GUARD
#define QUXLANG_QUERIES_STRUCT_DIRECT_BASES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/struct_inheritance.hpp>

#include <vector>

namespace quxlang
{
    /** Resolves and validates the active direct bases declared by one struct. */
    struct struct_direct_bases_query
    {
        static constexpr auto query_id = "struct_direct_bases";
        using input_type = type_symbol;
        using output_type = std::vector< struct_base_declaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_STRUCT_DIRECT_BASES_HEADER_GUARD
