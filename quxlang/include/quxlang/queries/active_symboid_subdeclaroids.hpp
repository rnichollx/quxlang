// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ACTIVE_SYMBOID_SUBDECLAROIDS_HEADER_GUARD
#define QUXLANG_QUERIES_ACTIVE_SYMBOID_SUBDECLAROIDS_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/basic_types.hpp>

#include <vector>

namespace quxlang
{
    /** Returns the active source declarations contained directly by one canonical symbol. */
    struct active_symboid_subdeclaroids_query
    {
        static constexpr auto query_id = "active_symboid_subdeclaroids";
        using input_type = type_symbol;
        using output_type = std::vector< subdeclaroid >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ACTIVE_SYMBOID_SUBDECLAROIDS_HEADER_GUARD
