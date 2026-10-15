// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_DECLARATIONS_HEADER_GUARD
#define QUXLANG_QUERIES_DECLARATIONS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>


namespace quxlang
{
    struct declarations_query
    {
        static constexpr auto query_id = "declarations";
        using input_type = type_symbol;
        using output_type = std::vector<declaration>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_DECLARATIONS_HEADER_GUARD
