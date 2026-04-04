// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SYMBOID_SUBDECLAROIDS_HEADER_GUARD
#define QUXLANG_QUERIES_SYMBOID_SUBDECLAROIDS_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>


namespace quxlang
{
    struct symboid_subdeclaroids_query
    {
        static constexpr auto query_id = "symboid_subdeclaroids";
        using input_type = type_symbol;
        using output_type = std::vector<subdeclaroid>;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SYMBOID_SUBDECLAROIDS_HEADER_GUARD
