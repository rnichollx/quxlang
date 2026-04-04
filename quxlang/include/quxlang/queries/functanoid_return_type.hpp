// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTANOID_RETURN_TYPE_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTANOID_RETURN_TYPE_HEADER_GUARD

#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct functanoid_return_type_query
    {
        static constexpr auto query_id = "functanoid_return_type";
        using input_type = instanciation_reference;
        using output_type = type_symbol;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTANOID_RETURN_TYPE_HEADER_GUARD
