// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_RELOCATABLE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_RELOCATABLE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Reports whether a value can be relocated by moving its stored bytes.
    struct type_is_trivially_relocatable_query
    {
        static constexpr auto query_id = "type_is_trivially_relocatable";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_RELOCATABLE_HEADER_GUARD
