// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_HEADER_GUARD

#include <quxlang/data/functanoid_requirements.hpp>

#include <set>

namespace quxlang
{
    struct functanoid_indirectly_instantiated_functanoids_query
    {
        static constexpr auto query_id = "functanoid_indirectly_instantiated_functanoids";
        using input_type = functanoid_requirement_input;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_INDIRECTLY_INSTANTIATED_FUNCTANOIDS_HEADER_GUARD
