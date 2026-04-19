// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_LIST_BUILTIN_CONSTRUCTORS_HEADER_GUARD
#define QUXLANG_QUERIES_LIST_BUILTIN_CONSTRUCTORS_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>
#include <quxlang/data/builtin_functions.hpp>
#include <set>


namespace quxlang
{
    struct list_builtin_constructors_query
    {
        static constexpr auto query_id = "list_builtin_constructors";
        using input_type = type_symbol;
        using output_type = std::set< builtin_function_info >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_LIST_BUILTIN_CONSTRUCTORS_HEADER_GUARD
