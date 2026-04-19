// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_PRIMITIVE_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_PRIMITIVE_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/builtin_functions.hpp>
#include <optional>


namespace quxlang
{
    struct function_primitive_query
    {
        static constexpr auto query_id = "function_primitive";
        using input_type = temploid_reference;
        using output_type = std::optional< builtin_function_info >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_PRIMITIVE_HEADER_GUARD
