// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_FUNCTUM_OVERLOADS_HEADER_GUARD
#define QUXLANG_QUERIES_FUNCTUM_OVERLOADS_HEADER_GUARD

#include <quxlang/data/tempar_types.hpp>
#include <quxlang/data/basic_types.hpp>
#include <set>


namespace quxlang
{
    struct functum_overloads_query
    {
        static constexpr auto query_id = "functum_overloads";
        using input_type = type_symbol;
        using output_type = std::set< temploid_ensig >;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_FUNCTUM_OVERLOADS_HEADER_GUARD
