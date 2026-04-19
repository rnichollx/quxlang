// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTION_POSITIONAL_PARAMETER_NAMES_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTION_POSITIONAL_PARAMETER_NAMES_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <optional>
#include <vector>
#include <string>


namespace quxlang
{
    struct function_positional_parameter_names_query
    {
        static constexpr auto query_id = "function_positional_parameter_names";
        using input_type = temploid_reference;
        using output_type = std::vector< std::optional< std::string > >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTION_POSITIONAL_PARAMETER_NAMES_HEADER_GUARD
