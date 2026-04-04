// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LIST_STATIC_TESTS_HEADER_GUARD
#define QUXLANG_QUERIES_LIST_STATIC_TESTS_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <set>


namespace quxlang
{
    struct list_static_tests_query
    {
        static constexpr auto query_id = "list_static_tests";
        using input_type = type_symbol;
        using output_type = std::set< type_symbol >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LIST_STATIC_TESTS_HEADER_GUARD
