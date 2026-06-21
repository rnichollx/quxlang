// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LIST_UNIT_TESTS_HEADER_GUARD
#define QUXLANG_QUERIES_LIST_UNIT_TESTS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>

#include <set>

namespace quxlang
{
    /**
     * Discovers UNIT_TEST declarations reachable from one module, namespace, or class.
     */
    struct list_unit_tests_query
    {
        static constexpr auto query_id = "list_unit_tests";
        using input_type = type_symbol;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LIST_UNIT_TESTS_HEADER_GUARD
