// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <optional>


namespace quxlang
{
    struct functum_select_function_query
    {
        static constexpr auto query_id = "functum_select_function";
        using input_type = initialization_reference;
        using output_type = std::optional< temploid_reference >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_SELECT_FUNCTION_HEADER_GUARD
