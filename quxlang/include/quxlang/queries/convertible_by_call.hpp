// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_CONVERTIBLE_BY_CALL_HEADER_GUARD
#define QUXLANG_QUERIES_CONVERTIBLE_BY_CALL_HEADER_GUARD

#include <quxlang/data/convertibility_types.hpp>
#include <quxlang/data/basic_types.hpp>
#include <optional>


namespace quxlang
{
    struct convertible_by_call_query
    {
        static constexpr auto query_id = "convertible_by_call";
        using input_type = implicitly_convertible_to_input;
        using output_type = std::optional<type_symbol>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_CONVERTIBLE_BY_CALL_HEADER_GUARD
