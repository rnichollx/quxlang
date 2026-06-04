// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_HEADER_GUARD
#define QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    /// Reports whether a type's default construction is equivalent to zero-initialized storage.
    struct type_is_trivially_default_constructible_query
    {
        static constexpr auto query_id = "type_is_trivially_default_constructible";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TYPE_IS_TRIVIALLY_DEFAULT_CONSTRUCTIBLE_HEADER_GUARD
