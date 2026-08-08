// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ACTIVE_SUBDECLAROIDS_HEADER_GUARD
#define QUXLANG_QUERIES_ACTIVE_SUBDECLAROIDS_HEADER_GUARD

#include <quxlang/ast2/ast2_entity.hpp>
#include <quxlang/data/basic_types.hpp>

#include <vector>

namespace quxlang
{
    /** Returns the active source declarations that define one canonical child symbol. */
    struct active_subdeclaroids_query
    {
        static constexpr auto query_id = "active_subdeclaroids";
        using input_type = type_symbol;
        using output_type = std::vector< subdeclaroid >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ACTIVE_SUBDECLAROIDS_HEADER_GUARD
