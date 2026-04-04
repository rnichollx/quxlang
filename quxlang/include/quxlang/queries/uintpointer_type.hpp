// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_UINTPOINTER_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_UINTPOINTER_TYPE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>
#include <variant>


namespace quxlang
{
    struct uintpointer_type_query
    {
        static constexpr auto query_id = "uintpointer_type";
        using input_type = std::monostate;
        using output_type = type_symbol;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_UINTPOINTER_TYPE_HEADER_GUARD
