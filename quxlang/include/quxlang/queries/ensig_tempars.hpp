// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_ENSIG_TEMPARS_HEADER_GUARD
#define QUXLANG_QUERIES_ENSIG_TEMPARS_HEADER_GUARD

#include <quxlang/data/tempar_types.hpp>
#include <quxlang/data/type_symbol.hpp>


namespace quxlang
{
    struct ensig_tempars_query
    {
        static constexpr auto query_id = "ensig_tempars";
        using input_type = temploid_ensig;
        using output_type = tempar_name_set;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_ENSIG_TEMPARS_HEADER_GUARD
