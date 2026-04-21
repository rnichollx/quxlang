// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLEX_BUILTIN_TEMPLATES_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLEX_BUILTIN_TEMPLATES_HEADER_GUARD

#include <set>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct templex_builtin_templates_query
    {
        static constexpr auto query_id = "templex_builtin_templates";
        using input_type = type_symbol;
        using output_type = std::set< temploid_ensig >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLEX_BUILTIN_TEMPLATES_HEADER_GUARD
