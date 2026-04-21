// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD
#define QUXLANG_QUERIES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD

#include <optional>

#include <quxlang/data/basic_types.hpp>

namespace quxlang
{
    struct templex_select_template_query
    {
        static constexpr auto query_id = "templex_select_template";
        using input_type = initialization_reference;
        using output_type = std::optional< temploid_reference >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_TEMPLEX_SELECT_TEMPLATE_HEADER_GUARD
