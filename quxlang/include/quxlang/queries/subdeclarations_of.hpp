// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_SUBDECLARATIONS_OF_HEADER_GUARD
#define QUXLANG_QUERIES_SUBDECLARATIONS_OF_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/ast2/ast2_entity.hpp>
#include <vector>


namespace quxlang
{
    /** Returns the stored subdeclarations in source order, including inactive declarations.
     * Symboids without a subdeclaration list produce an empty result.
     */
    struct subdeclarations_of_query
    {
        static constexpr auto query_id = "subdeclarations_of";
        using input_type = type_symbol;
        using output_type = std::vector< subdeclaration >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_SUBDECLARATIONS_OF_HEADER_GUARD
