// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_PARAM_NAMES_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_PARAM_NAMES_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct function_param_names_query
    {
        static constexpr auto query_id = "function_param_names";
        using input_type = temploid_reference;
        using output_type = param_names;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_PARAM_NAMES_HEADER_GUARD
