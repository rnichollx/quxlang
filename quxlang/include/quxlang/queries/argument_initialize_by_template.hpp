// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ARGUMENT_INITIALIZE_BY_TEMPLATE_HEADER_GUARD
#define QUXLANG_QUERIES_ARGUMENT_INITIALIZE_BY_TEMPLATE_HEADER_GUARD

#include <quxlang/data/argument_adaptation_types.hpp>
#include <quxlang/data/basic_types.hpp>
#include <optional>


namespace quxlang
{
    struct argument_initialize_by_template_query
    {
        static constexpr auto query_id = "argument_initialize_by_template";
        using input_type = argument_init_input;
        using output_type = std::optional<type_symbol>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ARGUMENT_INITIALIZE_BY_TEMPLATE_HEADER_GUARD
