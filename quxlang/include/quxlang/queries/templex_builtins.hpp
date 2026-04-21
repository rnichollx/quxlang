// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLEX_BUILTINS_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLEX_BUILTINS_HEADER_GUARD

#include <vector>

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/builtin_templates.hpp>

namespace quxlang
{
    struct templex_builtins_query
    {
        static constexpr auto query_id = "templex_builtins";
        using input_type = type_symbol;
        using output_type = std::vector< builtin_template_info >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLEX_BUILTINS_HEADER_GUARD
