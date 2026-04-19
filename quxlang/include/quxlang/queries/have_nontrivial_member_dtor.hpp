// Copyright 2026 Ryan P. Nicholl, rnicholl@protonmail.com

#ifndef QUXLANG_QUERIES_HAVE_NONTRIVIAL_MEMBER_DTOR_HEADER_GUARD
#define QUXLANG_QUERIES_HAVE_NONTRIVIAL_MEMBER_DTOR_HEADER_GUARD

#include <quxlang/data/basic_types.hpp>


namespace quxlang
{
    struct have_nontrivial_member_dtor_query
    {
        static constexpr auto query_id = "have_nontrivial_member_dtor";
        using input_type = type_symbol;
        using output_type = bool;
    };
} // namespace quxlang

#endif // QUXLANG_QUERIES_HAVE_NONTRIVIAL_MEMBER_DTOR_HEADER_GUARD
