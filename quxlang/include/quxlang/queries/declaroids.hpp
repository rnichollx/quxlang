// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_DECLAROIDS_HEADER_GUARD
#define QUXLANG_QUERIES_DECLAROIDS_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>


namespace quxlang
{
    struct declaroids_query
    {
        static constexpr auto query_id = "declaroids";
        using input_type = type_symbol;
        using output_type = std::vector<declaroid>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_DECLAROIDS_HEADER_GUARD
